#include "downloader.h"
#include <iostream>

int main() {
  std::string repo_id = "Qwen/Qwen2.5-0.5B-Instruct-GGUF";
  std::string filename = "qwen2.5-0.5b-instruct-q2_k.gguf";

  if (Downloader::hf_hub_download(repo_id, filename).success) {
    std::cout << "Downloaded " << filename << " from " << repo_id << std::endl;
  } else {
    std::cout << "Failed to download " << filename << " from " << repo_id
              << std::endl;
  }
}