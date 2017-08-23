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

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

// Audio output using ALSA.
class AudioOutputALSA {
 public:
  bool Start();

  void Stop();

  void Send(std::shared_ptr<std::vector<unsigned char>> data);

  friend void fill_audio(void* userdata, unsigned char* stream, int len);

 private:
  std::vector<std::shared_ptr<std::vector<unsigned char>>> audioData;
  std::mutex audioDataMutex;
  std::condition_variable audioDataCv;
  std::unique_ptr<std::thread> alsaThread;
  bool isRunning = false;
  std::mutex isRunningMutex;
};
