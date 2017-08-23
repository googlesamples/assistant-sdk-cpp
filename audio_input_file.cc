/*
Copyright 2017 Google Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "audio_input_file.h"

#include <fstream>
#include <iostream>

std::unique_ptr<std::thread> AudioInputFile::GetBackgroundThread() {
  return std::unique_ptr<std::thread>(new std::thread([this]() {
    // Initialize.
    std::ifstream file_stream(file_path_);
    if (!file_stream) {
      std::cerr << "AudioInputFile cannot open file " << file_path_ << std::endl;
      return;
    }

    const size_t chunk_size = 20 * 1024;  // 20KB
    std::shared_ptr<std::vector<unsigned char>> chunk(
        new std::vector<unsigned char>);
    chunk->resize(chunk_size);
    while (is_running_) {
      // Read another chunk from the file.
      std::streamsize bytes_read =
          file_stream.rdbuf()->sgetn((char*)&(*chunk.get())[0], chunk->size());
      if (bytes_read > 0) {
        chunk->resize(bytes_read);
        for (auto& listener : data_listeners_) {
          listener(chunk);
        }
      }
      if (bytes_read < chunk->size()) {
        break;
      }
      // Wait a second before writing the next chunk.
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Call |OnStop|.
    OnStop();
  }));
}
