// MIT License
//
// Copyright (c) 2025 Alejandro González Cantón
// Copyright (c) 2025 Miguel Ángel González Santamarta
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef HFHUB_CPP_H
#define HFHUB_CPP_H

#include <stdint.h>
#include <string>
#include <variant>

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

std::variant<struct FileMetadata, std::string>
get_model_metadata_from_hf(const std::string &repo, const std::string &file);

struct DownloadResult
hf_hub_download(const std::string &repo_id, const std::string &filename,
                const std::string &cache_dir = "~/.cache/huggingface/hub",
                bool force_download = false);

#endif // HFHUB_CPP_H
}