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

#include "audio_input.h"

#include <portaudio.h>

class AudioPA : public AudioInput {
 public:
  ~AudioPA() override {}

  virtual std::unique_ptr<std::thread> GetBackgroundThread() override;

  int Open();
  int Write(const void *buffer, unsigned long frames);
  void Stop();
  void Close();

 private:

  static constexpr int kInputNumChannels = 1;
  static constexpr int kOutputNumChannels = 1;

  // For 16000Hz, it's about 0.1 second.
  static constexpr double kSampleRate = 16000;
  static constexpr PaSampleFormat kSampleFormat = paInt16;
  static constexpr unsigned long kFramesPerBuffer = 512;

  PaStream* stream;

};
