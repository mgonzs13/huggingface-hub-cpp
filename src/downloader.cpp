#include "downloader.h"
#include <chrono>
#include <curl/curl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>
#include <sstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

long get_file_size(const std::string &filename) {
	struct stat stat_buf;
	if (stat(filename.c_str(), &stat_buf) == 0) {
			return stat_buf.st_size;
	}
	return 0; // File doesn't exist
}

std::string create_cache_system(const std::string &cache_dir, const std::string &repo_id) {
  std::string model_folder = std::string("models/" + repo_id);

  size_t pos = 0;
  while ((pos = model_folder.find("/", pos)) != std::string::npos) {
      model_folder.replace(pos, 1, "--");
      pos += 2;
  }

  std::string expanded_cache_dir = fs::absolute(fs::path(cache_dir));

  std::string model_cache_path = expanded_cache_dir + "/" + model_folder + "/";

  std::string refs_path = model_cache_path + std::string("refs");
  std::string blobs_path = model_cache_path + std::string("blobs");
  std::string snapshots_path = model_cache_path + std::string("snapshots");

  fs::create_directories(refs_path);
  fs::create_directories(blobs_path);
  fs::create_directories(snapshots_path);

  return model_cache_path;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
  std::ofstream *out = static_cast<std::ofstream *>(stream);
  out->write(static_cast<char *>(ptr), size * nmemb);
  return size * nmemb;
}

// Progress bar function
int progress_callback(void *ptr, curl_off_t total, curl_off_t now, curl_off_t,
                      curl_off_t) {
  static auto start_time = std::chrono::steady_clock::now();
  if (total > 0) {
    int width = 30; // Progress bar width
    float percent = static_cast<float>(now) / total;
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

    // Print speed and ETA
    std::cout << " " << (speed / 1024 / 1024)
              << " MB/s | ETA: " << static_cast<int>(remaining) << "s   "
              << std::flush;
  }
  return 0; // Continue downloading
}

bool Downloader::hf_hub_download(const std::string &repo_id,
                                 const std::string &filename,
                                 const std::string &cache_dir,
                                 const std::string &local_dir,
                                 bool force_download) {
  
  // If model exists, return true
  if (fs::exists(local_dir)) {
    return true;
  }
  
  CURL *curl = curl_easy_init();
  if (!curl) {
    return false;
  }

  // 1. Create Cache Dir Struct
  std::string cache_dir = create_cache_system(cache_dir, repo_id);

  // 2. Check if model file exist
  fs::path blob_file_path;
  fs::path snapshot_file_path(cache_dir + "snapshots/" + filename)
  bool file_has_been_downloaded = false;

  if (!fs::exists(snapshot_file_path)) {
    // Create blob file
    blob_file_path = fs::path(cache_dir + "blobs/" + filename);

    // Create refs file


    // Create snapshot file
    fs::create_symlink(blob_file_path, snapshot_file_path)
  } else {
    // Get the blob file
    blob_file_path = fs::read_symlink(snapshot_file_path);
  }
  // Set file path to the blob

  // 3. Download the file
	std::string url = "https://huggingface.co/" + repo_id + "/resolve/main/" + filename;

  long existing_size = get_file_size(blob_file_path);
	std::ofstream file(blob_file_path, std::ios::binary | std::ios::app); // Append mode

  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << blob_file_path << std::endl;
    curl_easy_cleanup(curl);
    return false;
  }

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, nullptr);

	if (existing_size > 0 && !force_download) {
		curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)existing_size);
		std::cout << "Resuming download from " << existing_size << " bytes...\n";
	}

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  std::cout << "\nDownloaded to: " << expanded_path << std::endl;
  std::cout << std::endl; // Move to the next line after download
  return res == CURLE_OK;
}