#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <string>

class Downloader {
public:
  static bool hf_hub_download(const std::string &repo_id,
                              const std::string &filename,
                              const std::string &cache_dir = "~/.cache/huggingface",
                              const std::string &local_dir = "",
                              bool force_download = false);
};

#endif // DOWNLOADER_H
