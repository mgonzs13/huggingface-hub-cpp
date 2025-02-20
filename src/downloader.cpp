#include "downloader.h"
#include <chrono>
#include <curl/curl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>

long get_file_size(const std::string &filename) {
	struct stat stat_buf;
	if (stat(filename.c_str(), &stat_buf) == 0) {
			return stat_buf.st_size;
	}
	return 0; // File doesn't exist
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
    int width = 50; // Progress bar width
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
  CURL *curl = curl_easy_init();
  if (!curl)
    return false;


	std::string output_path = local_dir + filename;
	long existing_size = get_file_size(output_path);

	std::string url = "https://huggingface.co/" + repo_id + "/resolve/main/" + filename;
	std::ofstream file(output_path, std::ios::binary | std::ios::app); // Append mode

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
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

  std::cout << std::endl; // Move to the next line after download
  return res == CURLE_OK;
}