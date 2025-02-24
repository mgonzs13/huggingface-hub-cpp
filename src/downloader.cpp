#include "downloader.h"
#include <algorithm>
#include <chrono>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

using namespace huggingface_hub;

long get_file_size(const std::string &filename) {
  struct stat stat_buf;
  if (stat(filename.c_str(), &stat_buf) == 0) {
    return stat_buf.st_size;
  }
  return 0; // File doesn't exist
}

std::filesystem::path expand_user_home(const std::string &path) {
  if (!path.empty() && path[0] == '~') {
    const char *home = std::getenv("HOME"); // Get HOME environment variable
    if (home) {
      return std::filesystem::path(home + path.substr(1));
    }
  }
  return std::filesystem::path(path);
}

std::string create_cache_system(const std::string &cache_dir,
                                const std::string &repo_id) {
  std::string model_folder = std::string("models/" + repo_id);

  size_t pos = 0;
  while ((pos = model_folder.find("/", pos)) != std::string::npos) {
    model_folder.replace(pos, 1, "--");
    pos += 2;
  }

  std::string expanded_cache_dir = expand_user_home(cache_dir);

  std::string model_cache_path = expanded_cache_dir + "/" + model_folder + "/";

  std::string refs_path = model_cache_path + std::string("refs");
  std::string blobs_path = model_cache_path + std::string("blobs");
  std::string snapshots_path = model_cache_path + std::string("snapshots");

  std::filesystem::create_directories(refs_path);
  std::filesystem::create_directories(blobs_path);
  std::filesystem::create_directories(snapshots_path);

  return model_cache_path;
}

size_t write_string_data(void *ptr, size_t size, size_t nmemb, void *stream) {
  if (!stream) {
    std::cerr << "Error: stream is null!" << std::endl;
    return 0;
  }
  std::string *out = static_cast<std::string *>(stream);
  out->append(static_cast<char *>(ptr), size * nmemb);
  return size * nmemb;
}

size_t write_file_data(void *ptr, size_t size, size_t nmemb, void *stream) {
  if (!stream) {
    std::cerr << "Error: stream is null!" << std::endl;
    return 0;
  }
  std::ofstream *out = static_cast<std::ofstream *>(stream);
  if (!out->is_open()) {
    std::cerr << "Error: output file stream is not open!" << std::endl;
    return 0;
  }
  out->write(static_cast<char *>(ptr), size * nmemb);
  return size * nmemb;
}

// Function to extract SHA256 from Git LFS metadata
std::string extract_SHA256(const std::string &response) {
  std::istringstream stream(response);
  std::string line;

  while (std::getline(stream, line)) {
    if (line.find("oid sha256:") != std::string::npos) {
      auto result =
          line.substr(line.find("sha256:") + 7); // Extract after "sha256:"
      stream.str("");                            // Clear the stream
      return result;
    }
  }
  return ""; // Return empty if not found
}

uint64_t extract_file_size(const std::string &response) {
  std::istringstream stream(response);
  std::string line;
  uint64_t size = 0;

  while (std::getline(stream, line)) {
    if (line.find("size ") != std::string::npos) {
      std::string sizeStr = line.substr(5); // Extract after "size "
      size = std::stoull(sizeStr);
      return size;
    }
  }
  return 0; // Return empty if not found
}

struct FileMetadata get_metadata_from_hf(const std::string &repo,
                                         const std::string &file) {
  struct FileMetadata metadata;
  std::string url = "https://huggingface.co/" + repo + "/raw/main/" + file;
  std::string response, headers;

  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Failed to initialize CURL\n";
    return metadata;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_string_data);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    std::cerr << "CURL request failed: " << curl_easy_strerror(res) << "\n";
    return metadata;
  }

  std::string line;
  metadata.sha256 = extract_SHA256(response);
  metadata.size = extract_file_size(response);

  std::istringstream header_stream(headers);
  while (std::getline(header_stream, line)) {
    if (line.find("x-repo-commit:") != std::string::npos) {
      metadata.commit = line.substr(line.find(":") + 2);
      break;
    }
  }
  metadata.commit.erase(metadata.commit.find_last_not_of(" \n\r\t") +
                        1); // Trim trailing whitespace

  std::cout << "SHA256: " << metadata.sha256 << std::endl;
  std::cout << "Commit: " << metadata.commit << std::endl;
  std::cout << "Size: " << metadata.size << std::endl;

  return metadata;
}

// Progress bar function
int progress_callback(void *userdata, curl_off_t total, curl_off_t now,
                      curl_off_t, curl_off_t) {
  static auto start_time = std::chrono::steady_clock::now();
  struct FileMetadata *metadata = static_cast<struct FileMetadata *>(userdata);
  uint64_t size = metadata->size;
  uint64_t byte_offset = total - size;
  uint64_t downloaded = now - byte_offset;

  if (total > 0) {
    int width = 30; // Progress bar width
    float percent = static_cast<float>(downloaded) / size;
    int filled = static_cast<int>(percent * width);

    auto elapsed = std::chrono::steady_clock::now() - start_time;
    double speed = now / (std::chrono::duration<double>(elapsed).count() +
                          1e-6); // Avoid division by zero
    double remaining = (total - now) / speed;

    // Print progress bar
    std::cout << "\r[";
    for (int i = 0; i < filled; ++i)
      std::cout << "#";
    for (int i = filled; i < width; ++i)
      std::cout << " ";
    std::cout << "] " << std::fixed << std::setprecision(2) << (percent * 100)
              << "%";

    std::cout << "   " << downloaded / 1024 / 1024 << " MB / "
              << size / 1024 / 1024 << " MB";

    // Print speed and ETA
    std::cout << " " << (speed / 1024 / 1024)
              << " MB/s | ETA: " << static_cast<int>(remaining) << "s   "
              << std::flush;
  }
  return 0; // Continue downloading
}

struct DownloadResult Downloader::hf_hub_download(const std::string &repo_id,
                                                  const std::string &filename,
                                                  const std::string &cache_dir,
                                                  bool force_download) {

  struct DownloadResult result;
  result.success = true;

  CURL *curl = curl_easy_init();
  if (!curl) {
    result.success = false;
    return result;
  }

  // 1. Create Cache Dir Struct
  std::string cache_model_dir = create_cache_system(cache_dir, repo_id);
  std::cout << "Cache directory: " << cache_model_dir << std::endl;
  std::cout << "Downloading " << filename << " from " << repo_id << std::endl;

  // 2. Check if model file exist
  struct FileMetadata metadata = get_metadata_from_hf(repo_id, filename);
  std::filesystem::path blob_file_path(cache_model_dir + "blobs/" +
                                       metadata.sha256);
  std::filesystem::path blob_incomplete_file_path(
      cache_model_dir + "blobs/" + metadata.sha256 + ".incomplete");
  std::filesystem::path snapshot_file_path(cache_model_dir + "snapshots/" +
                                           metadata.commit + "/" + filename);
  std::filesystem::path refs_file_path(cache_model_dir + "refs/main");

  result.path = snapshot_file_path;

  if (std::filesystem::exists(blob_file_path) && !force_download) {
    std::cout << "Blob file exists. Skipping download..." << std::endl;
    return result;
  }

  if (std::filesystem::exists(refs_file_path)) {
    std::ifstream refs_file(refs_file_path);
    std::string commit;
    refs_file >> commit;
    refs_file.close();
  } else {
    std::ofstream refs_file(refs_file_path);
    refs_file << metadata.commit;
    refs_file.close();
  }

  // 3. Download the file
  std::string url =
      "https://huggingface.co/" + repo_id + "/resolve/main/" + filename;

  long existing_size = get_file_size(blob_incomplete_file_path);
  std::ofstream file(blob_incomplete_file_path,
                     std::ios::binary | std::ios::app);

  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << blob_incomplete_file_path
              << std::endl;
    curl_easy_cleanup(curl);
    result.success = false;
    return result;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());   // Set URL
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_file_data);                // Write data to file
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file); // File stream
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);   // Enable progress callback
  curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
                   progress_callback); // Progress callback
  curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &metadata);

  if (existing_size > 0 && !force_download) {
    curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE,
                     (curl_off_t)existing_size); // Resume download
    std::cout << "Resuming download from " << existing_size << " bytes...\n";
  }

  CURLcode res = curl_easy_perform(curl);
  std::cout << std::endl; // Move to the next line after download

  std::filesystem::create_directories(snapshot_file_path.parent_path());

  if (res != CURLE_OK) {
    std::cerr << "CURL request failed: " << curl_easy_strerror(res) << "\n";
    file.close();
    curl_easy_cleanup(curl);
    result.success = false;
    return result;
  }

  if (std::filesystem::exists(snapshot_file_path)) {
    std::cout << "Snapshot file exists. Deleting..." << std::endl;
    std::filesystem::remove(snapshot_file_path);
  }
  std::filesystem::rename(blob_incomplete_file_path, blob_file_path);
  std::filesystem::create_symlink(blob_file_path, snapshot_file_path);

  file.close();
  curl_easy_cleanup(curl);

  std::cout << "Downloaded to: " << snapshot_file_path << std::endl;

  result.success = res == CURLE_OK;
  return result;
}