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

#include "audio_input_alsa.h"

#include <alsa/asoundlib.h>

#include <iostream>

std::unique_ptr<std::thread> AudioInputALSA::GetBackgroundThread() {
  return std::unique_ptr<std::thread>(new std::thread([this]() {
    // Initialize.
    snd_pcm_t* pcm_handle;
    int pcm_open_ret = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (pcm_open_ret < 0) {
      std::cerr << "AudioInputALSA snd_pcm_open returned " << pcm_open_ret << std::endl;
      return;
    }
    int pcm_nonblock_ret = snd_pcm_nonblock(pcm_handle, SND_PCM_NONBLOCK);
    if (pcm_nonblock_ret < 0) {
      std::cerr << "AudioInputALSA snd_pcm_nonblock returned " << pcm_nonblock_ret << std::endl;
      return;
    }
    snd_pcm_hw_params_t* pcm_params;
    int malloc_param_ret = snd_pcm_hw_params_malloc(&pcm_params);
    if (malloc_param_ret < 0) {
      std::cerr << "AudioInputALSA snd_pcm_hw_params_malloc returned " << malloc_param_ret
          << std::endl;
      return;
    }
    snd_pcm_hw_params_any(pcm_handle, pcm_params);
    int set_param_ret =
        snd_pcm_hw_params_set_access(pcm_handle, pcm_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (set_param_ret < 0) {
      std::cerr << "AudioInputALSA snd_pcm_hw_params_set_access returned " << set_param_ret
          << std::endl;
      return;
    }
    set_param_ret =
        snd_pcm_hw_params_set_format(pcm_handle, pcm_params, SND_PCM_FORMAT_S16_LE);
    if (set_param_ret < 0) {
      std::cerr << "AudioInputALSA snd_pcm_hw_params_set_format returned " << set_param_ret
          << std::endl;
      return;
    }
    set_param_ret =
        snd_pcm_hw_params_set_channels(pcm_handle, pcm_params, 1);
    if (set_param_ret < 0) {
      std::cerr << "AudioInputALSA snd_pcm_hw_params_set_channels returned " << set_param_ret
          << std::endl;
      return;
    }
    unsigned int rate = 16000;
    set_param_ret =
        snd_pcm_hw_params_set_rate_near(pcm_handle, pcm_params, &rate, nullptr);
    if (set_param_ret < 0) {
      std::cerr << "AudioInputALSA snd_pcm_hw_params_set_rate_near returned " << set_param_ret
          << std::endl;
      return;
    }
    set_param_ret = snd_pcm_hw_params(pcm_handle, pcm_params);
    if (set_param_ret < 0) {
      std::cerr << "AudioInputALSA snd_pcm_hw_params returned " << set_param_ret << std::endl;
      return;
    }
    snd_pcm_hw_params_free(pcm_params);

    while (is_running_) {
      std::shared_ptr<std::vector<unsigned char>> audio_data(
            new std::vector<unsigned char>(kFramesPerPacket * kBytesPerFrame));
      int pcm_read_ret = snd_pcm_readi(pcm_handle, &(*audio_data.get())[0], kFramesPerPacket);
      if (pcm_read_ret == -EAGAIN) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      } else if (pcm_read_ret < 0) {
        std::cerr << "AudioInputALSA snd_pcm_readi returned " << pcm_read_ret << std::endl;
        break;
      } else if (pcm_read_ret > 0) {
        audio_data->resize(kBytesPerFrame * pcm_read_ret);
        for (auto& listener : data_listeners_) {
          listener(audio_data);
        }
      }
    }

    // Finalize.
    snd_pcm_close(pcm_handle);

    // Call |OnStop|.
    OnStop();
  }));
}
