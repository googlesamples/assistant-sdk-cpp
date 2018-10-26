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

#ifndef SRC_ASSISTANT_AUDIO_INPUT_ALSA_H_
#define SRC_ASSISTANT_AUDIO_INPUT_ALSA_H_

#include "assistant/audio_input.h"

#include <memory>

class AudioInputALSA : public AudioInput {
 public:
  ~AudioInputALSA() override {}

  std::unique_ptr<std::thread> GetBackgroundThread() override;

 private:
  // For 16000Hz, it's about 0.1 second.
  static constexpr int kFramesPerPacket = 1600;
  // 1 channel, S16LE, so 2 bytes each frame.
  static constexpr int kBytesPerFrame = 2;
};

#endif  // SRC_ASSISTANT_AUDIO_INPUT_ALSA_H_
