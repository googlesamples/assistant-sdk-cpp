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

#include "audio_pa.h"

#include <iostream>
#include <string>

std::unique_ptr<std::thread> AudioPA::GetBackgroundThread() {
  return std::unique_ptr<std::thread>(new std::thread([this]() {

    PaError err;

    std::shared_ptr<std::vector<unsigned char>> audio_data(
        new std::vector<unsigned char>);

    const size_t chunk_size = kFramesPerBuffer * kInputNumChannels * sizeof(short);
        
    audio_data->resize(chunk_size);

    err = Pa_StartStream( stream );
    if (err != paNoError) goto error;

    while (is_running_) {

      err = Pa_ReadStream(stream, &(*audio_data.get())[0], kFramesPerBuffer);
      if(err == paNoError) {
        for (auto& listener : data_listeners_) {
          listener(audio_data);
        }
      }
      else if(err != paInputOverflowed) {
        std::cerr << "Pa_ReadStream error: ";
          goto error;
      }
    }

    // Call |OnStop|.
    OnStop();
    return;

error:
    std::cerr << Pa_GetErrorText(err) << std::endl;
    
    // Call |OnStop|.
    OnStop();
  }));
}


int AudioPA::Open() {

  PaError err;
  PaDeviceIndex devInput, devOutput;
  const PaDeviceInfo* inputInfo;
  const PaDeviceInfo* outputInfo;
  PaStreamParameters inputParameters;
  PaStreamParameters outputParameters;

  err = Pa_Initialize();
  if(err != paNoError) goto error;

  devInput = Pa_GetDefaultInputDevice();
  inputInfo = Pa_GetDeviceInfo( devInput );
  std::cout << "Input device # " << devInput << std::endl;
  std::cout << "    Name: " << inputInfo->name << std::endl;

  inputParameters.device = devInput;
  inputParameters.channelCount = kInputNumChannels;
  inputParameters.sampleFormat =  kSampleFormat;
  inputParameters.suggestedLatency = inputInfo->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = NULL;

  devOutput = Pa_GetDefaultOutputDevice();
  outputInfo = Pa_GetDeviceInfo( devOutput );
  std::cout << "Output device # " << devOutput << std::endl;
  std::cout << "    Name: " << outputInfo->name << std::endl;

  outputParameters.device = devOutput;
  outputParameters.channelCount = kOutputNumChannels;
  outputParameters.sampleFormat =  kSampleFormat;
  outputParameters.suggestedLatency = outputInfo->defaultLowOutputLatency;
  outputParameters.hostApiSpecificStreamInfo = NULL;

  err = Pa_IsFormatSupported( &inputParameters, &outputParameters, kSampleRate);
  if( err != paNoError ) goto error;

  err = Pa_OpenStream(
          &stream,
          &inputParameters,
          &outputParameters,
          kSampleRate,
          kFramesPerBuffer,
          paClipOff,
          NULL,
          NULL );
  if( err != paNoError ) goto error;

  return err;

error:
  if( stream ) {
      Pa_AbortStream( stream );
      Pa_CloseStream( stream );
  }
  Pa_Terminate();
  std::cerr << Pa_GetErrorText(err) << std::endl;
  return (int)err;
}

int AudioPA::Write( const void *buffer, unsigned long frames ) {
  return (int)Pa_WriteStream( stream, buffer, frames );
}

void AudioPA::Stop() {

  if( stream ) {
      Pa_StopStream( stream );
  }
}

void AudioPA::Close() {

  if( stream ) {
      Pa_AbortStream( stream );
      Pa_CloseStream( stream );
  }
  Pa_Terminate();
}
