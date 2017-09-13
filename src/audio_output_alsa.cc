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

#include "audio_output_alsa.h"

#include <alsa/asoundlib.h>

#include <iostream>

bool AudioOutputALSA::Start() {
  std::unique_lock<std::mutex> lock(isRunningMutex);

  if (isRunning) {
    return true;
  }

  snd_pcm_t* pcm_handle;
  int pcm_open_ret = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (pcm_open_ret < 0) {
    std::cerr << "AudioOutputALSA snd_pcm_open returned " << pcm_open_ret << std::endl;
    return false;
  }
  snd_pcm_hw_params_t* pcm_params;
  int malloc_param_ret = snd_pcm_hw_params_malloc(&pcm_params);
  if (malloc_param_ret < 0) {
    std::cerr << "AudioOutputALSA snd_pcm_hw_params_malloc returned " << malloc_param_ret
        << std::endl;
    return false;
  }
  snd_pcm_hw_params_any(pcm_handle, pcm_params);
  int set_param_ret =
      snd_pcm_hw_params_set_access(pcm_handle, pcm_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  if (set_param_ret < 0) {
    std::cerr << "AudioOutputALSA snd_pcm_hw_params_set_access returned " << set_param_ret
        << std::endl;
    return false;
  }
  set_param_ret =
      snd_pcm_hw_params_set_format(pcm_handle, pcm_params, SND_PCM_FORMAT_S16_LE);
  if (set_param_ret < 0) {
    std::cerr << "AudioOutputALSA snd_pcm_hw_params_set_format returned " << set_param_ret
        << std::endl;
    return false;
  }
  set_param_ret =
      snd_pcm_hw_params_set_channels(pcm_handle, pcm_params, 1);
  if (set_param_ret < 0) {
    std::cerr << "AudioOutputALSA snd_pcm_hw_params_set_channels returned " << set_param_ret
        << std::endl;
    return false;
  }
  unsigned int rate = 16000;
  set_param_ret =
      snd_pcm_hw_params_set_rate_near(pcm_handle, pcm_params, &rate, nullptr);
  if (set_param_ret < 0) {
    std::cerr << "AudioOutputALSA snd_pcm_hw_params_set_rate_near returned " << set_param_ret
        << std::endl;
    return false;
  }
  set_param_ret = snd_pcm_hw_params(pcm_handle, pcm_params);
  if (set_param_ret < 0) {
    std::cerr << "AudioOutputALSA snd_pcm_hw_params returned " << set_param_ret << std::endl;
    return false;
  }
  snd_pcm_hw_params_free(pcm_params);

  isRunning = true;
  alsaThread.reset(new std::thread([this, pcm_handle]() {
    while (isRunning) {
      std::unique_lock<std::mutex> lock(audioDataMutex);
      while (audioData.size() == 0 && isRunning) {
        audioDataCv.wait_for(lock, std::chrono::milliseconds(100));
      }
      if (!isRunning) {
        break;
      }

      std::shared_ptr<std::vector<unsigned char>> data = audioData[0];
      audioData.erase(audioData.begin());
      int frames = data->size() / 2;  // 1 channel, S16LE, so 2 bytes each frame.
      int pcm_write_ret = snd_pcm_writei(pcm_handle, &(*data.get())[0], frames);
      if (pcm_write_ret < 0) {
        int pcm_recover_ret = snd_pcm_recover(pcm_handle, pcm_write_ret, 0);
        if (pcm_recover_ret < 0) {
          std::cerr << "AudioOutputALSA snd_pcm_recover returns " << pcm_recover_ret << std::endl;
          break;
        }
      }
    }
    // Wait for all data to be consumed.
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
  }));
  std::cout << "AudioOutputALSA Start() succeeded" << std::endl;
  return true;
}

void AudioOutputALSA::Stop() {
  std::unique_lock<std::mutex> lick(isRunningMutex);

  if (!isRunning) {
    return;
  }

  isRunning = false;
  alsaThread->join();
  alsaThread.reset(nullptr);
  std::cout << "AudioOutputALSA Stop() succeeded" << std::endl;
}

void AudioOutputALSA::Send(std::shared_ptr<std::vector<unsigned char>> data) {
  std::unique_lock<std::mutex> lock(audioDataMutex);
  audioData.push_back(data);
  audioDataCv.notify_one();
}
