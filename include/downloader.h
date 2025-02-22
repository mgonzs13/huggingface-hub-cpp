#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <stdint.h>
#include <string>

namespace huggingface_hub {

struct FileMetadata {
  std::string sha256;
  std::string commit;
  uint64_t size;
};

struct DownloadResult {
  bool success;
  std::string path;
};

class Downloader {
public:
  static struct DownloadResult
  hf_hub_download(const std::string &repo_id, const std::string &filename,
                  const std::string &cache_dir = "~/.cache/huggingface/hub",
                  bool force_download = false);
};

#endif // DOWNLOADER_H
}