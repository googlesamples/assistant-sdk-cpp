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

#ifndef AUDIO_INPUT_H
#define AUDIO_INPUT_H

#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>

// Base class for audio input. Input data should be mono, s16_le, 16000kz.
// This class uses a separate thread to send audio data to listeners.
class AudioInput {
 public:
  virtual ~AudioInput() {}

  // Listeners might be called in different thread.
  void AddDataListener(
      std::function<void(std::shared_ptr<std::vector<unsigned char>>)>
      listener) {
    data_listeners_.push_back(listener);
  }
  void AddStopListener(std::function<void()> listener) {
    stop_listeners_.push_back(listener);
  }

  // This thread should:
  // 1. Initialize necessary resources;
  // 2. If |is_running_| is still true, keep sending audio data;
  // 3. Finalize necessary resources.
  // 4. Call |OnStop|.
  virtual std::unique_ptr<std::thread> GetBackgroundThread() = 0;

  // Asynchronously starts audio input. Starts internal thread to send audio.
  void Start() {
    std::unique_lock<std::mutex> lock(is_running_mutex_);
    if (is_running_) {
      return;
    }

    send_thread_ = std::move(GetBackgroundThread());
    is_running_ = true;
    return;
  }

  // Synchronously stops audio input.
  void Stop() {
    std::unique_lock<std::mutex> lock(is_running_mutex_);
    if (!is_running_) {
      return;
    }
    is_running_ = false;
    // |send_thread_| might have finished.
    if (send_thread_->joinable()) {
      send_thread_->join();
    }
  }

  // Whether audio input is being sent to listeners.
  bool IsRunning() {
    std::unique_lock<std::mutex> lock(is_running_mutex_);
    return is_running_;
  }

 protected:
  // Function to call when audio input is stopped.
  void OnStop() {
    for (auto& stop_listener : stop_listeners_) {
      stop_listener();
    }
  }

  // Listeners which will be called when audio input data arrives.
  std::vector<std::function<void(std::shared_ptr<std::vector<unsigned char>>)>>
      data_listeners_;

  // Whether audio input is being sent to listeners.
  bool is_running_ = false;

 private:
  std::vector<std::function<void()>> stop_listeners_;
  std::mutex is_running_mutex_;
  std::unique_ptr<std::thread> send_thread_;
};

#endif
