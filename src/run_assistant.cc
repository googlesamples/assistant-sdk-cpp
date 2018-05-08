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

#include <grpc++/grpc++.h>

#include <getopt.h>

#include <chrono>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef __linux__
#define ENABLE_ALSA
#endif

#ifdef ENABLE_ALSA
#include "audio_input_alsa.h"
#include "audio_output_alsa.h"
#endif

#include "google/assistant/embedded/v1alpha2/embedded_assistant.pb.h"
#include "google/assistant/embedded/v1alpha2/embedded_assistant.grpc.pb.h"

#include "assistant_config.h"
#include "audio_input.h"
#include "audio_input_file.h"
#include "json_util.h"

using google::assistant::embedded::v1alpha2::EmbeddedAssistant;
using google::assistant::embedded::v1alpha2::AssistRequest;
using google::assistant::embedded::v1alpha2::AssistResponse;
using google::assistant::embedded::v1alpha2::AudioInConfig;
using google::assistant::embedded::v1alpha2::AudioOutConfig;
using google::assistant::embedded::v1alpha2::AssistResponse_EventType_END_OF_UTTERANCE;

using grpc::CallCredentials;
using grpc::Channel;
using grpc::ClientReaderWriter;

static const std::string kCredentialsTypeUserAccount = "USER_ACCOUNT";
static const std::string kALSAAudioInput = "ALSA_INPUT";
static const std::string kLanguageCode = "en-US";
static const std::string kDeviceInstanceId = "default";
static const std::string kDeviceModelId = "default";

bool verbose = false;

// Creates a channel to be connected to Google.
std::shared_ptr<Channel> CreateChannel(const std::string& host) {
  std::ifstream file("robots.pem");
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string roots_pem = buffer.str();

  if (verbose) {
    std::clog << "assistant_sdk robots_pem: " << roots_pem << std::endl;
  }
  ::grpc::SslCredentialsOptions ssl_opts = {roots_pem, "", ""};
  auto creds = ::grpc::SslCredentials(ssl_opts);
  std::string server = host + ":443";
  if (verbose) {
    std::clog << "assistant_sdk CreateCustomChannel(" << server << ", creds, arg)"
      << std::endl << std::endl;
  }
  ::grpc::ChannelArguments channel_args;
  return CreateCustomChannel(server, creds, channel_args);
}

void PrintUsage() {
  std::cerr << "Usage: ./run_assistant "
            << "[--audio_input [<" << kALSAAudioInput << ">|<audio_file>] OR --text_input <string>] "
            << "--credentials_file <credentials_file> "
            << "[--credentials_type <" << kCredentialsTypeUserAccount << ">] "
            << "[--api_endpoint <API endpoint>] "
            << "[--locale <locale>]"
            << std::endl;
}

bool GetCommandLineFlags(
    int argc, char** argv, std::string* audio_input, std::string* text_input,
    std::string* credentials_file_path, std::string* credentials_type,
    std::string* api_endpoint, std::string* locale) {
  const struct option long_options[] = {
    {"audio_input",      required_argument, nullptr, 'i'},
    {"text_input",       required_argument, nullptr, 't'},
    {"credentials_file", required_argument, nullptr, 'f'},
    {"credentials_type", required_argument, nullptr, 'c'},
    {"api_endpoint",     required_argument, nullptr, 'e'},
    {"locale",           required_argument, nullptr, 'l'},
    {"verbose",          no_argument, nullptr, 'v'},
    {nullptr, 0, nullptr, 0}
  };
  *api_endpoint = ASSISTANT_ENDPOINT;
  while (true) {
    int option_index;
    int option_char =
        getopt_long(argc, argv, "i:t:f:c:e:l:v", long_options, &option_index);
    if (option_char == -1) {
      break;
    }
    switch (option_char) {
      case 'i':
        *audio_input = optarg;
        break;
      case 't':
        *text_input = optarg;
      case 'f':
        *credentials_file_path = optarg;
        break;
      case 'c':
        *credentials_type = optarg;
        if (*credentials_type != kCredentialsTypeUserAccount) {
          std::cerr << "Invalid credentials_type: \"" << *credentials_type
                    << "\". Should be \"" << kCredentialsTypeUserAccount
                    << "\"" << std::endl;
          return false;
        }
        break;
      case 'e':
        *api_endpoint = optarg;
        break;
      case 'l':
        *locale = optarg;
        break;
      case 'v':
        verbose = true;
        break;
      default:
        PrintUsage();
        return false;
    }
  }
  return true;
}

int main(int argc, char** argv) {
  std::string audio_input_source, text_input_source, credentials_file_path, credentials_type,
              api_endpoint, locale;
  // Initialize gRPC and DNS resolvers
  // https://github.com/grpc/grpc/issues/11366#issuecomment-328595941
  grpc_init();
  if (!GetCommandLineFlags(argc, argv, &audio_input_source, &text_input_source,
                           &credentials_file_path, &credentials_type,
                           &api_endpoint, &locale)) {
    return -1;
  }

  if (credentials_type.empty()) {
    credentials_type = kCredentialsTypeUserAccount; // Default is USER_ACCOUNT
  }

  // Create an AssistRequest
  AssistRequest request;
  auto* assist_config = request.mutable_config();

  if (locale.empty()) {
    locale = kLanguageCode; // Default locale
  }
  std::cout << "Using locale " << locale << std::endl;
  // Set the DialogStateIn of the AssistRequest
  assist_config->mutable_dialog_state_in()->set_language_code(locale);

  // Set the DeviceConfig of the AssistRequest
  assist_config->mutable_device_config()->set_device_id(kDeviceInstanceId);
  assist_config->mutable_device_config()->set_device_model_id(kDeviceModelId);

  // Set parameters for audio output
  assist_config->mutable_audio_out_config()->set_encoding(
    AudioOutConfig::LINEAR16);
  assist_config->mutable_audio_out_config()->set_sample_rate_hertz(16000);

  std::unique_ptr<AudioInput> audio_input;
  if (!audio_input_source.empty()) {
    // Set the AudioInConfig of the AssistRequest
    assist_config->mutable_audio_in_config()->set_encoding(
      AudioInConfig::LINEAR16);
    assist_config->mutable_audio_in_config()->set_sample_rate_hertz(16000);
  } else if (!text_input_source.empty()) {
    assist_config->set_text_query(text_input_source);
  } else {
    std::cerr << "requires either --audio_input or --text_input" << std::endl;
    return -1;
  }

  // Read credentials file.
  std::ifstream credentials_file(credentials_file_path);
  if (!credentials_file) {
    std::cerr << "Credentials file \"" << credentials_file_path
              << "\" does not exist." << std::endl;
    return -1;
  }
  std::stringstream credentials_buffer;
  credentials_buffer << credentials_file.rdbuf();
  std::string credentials = credentials_buffer.str();
  std::shared_ptr<CallCredentials> call_credentials;
  call_credentials = grpc::GoogleRefreshTokenCredentials(credentials);
  if (call_credentials.get() == nullptr) {
    std::cerr << "Credentials file \"" << credentials_file_path
              << "\" is invalid. Check step 5 in README for how to get valid "
              << "credentials." << std::endl;
    return -1;
  }

  // Begin a stream.
  auto channel = CreateChannel(api_endpoint);
  std::unique_ptr<EmbeddedAssistant::Stub> assistant(
      EmbeddedAssistant::NewStub(channel));

  grpc::ClientContext context;
  context.set_fail_fast(false);
  context.set_credentials(call_credentials);

  std::shared_ptr<ClientReaderWriter<AssistRequest, AssistResponse>>
      stream(std::move(assistant->Assist(&context)));
  // Write config in first stream.
  if (verbose) {
    std::clog << "assistant_sdk wrote first request: "
              << request.ShortDebugString() << std::endl;
  }
  stream->Write(request);

  if (!audio_input_source.empty()) {
    if (audio_input_source == kALSAAudioInput) {
#ifdef ENABLE_ALSA
        audio_input.reset(new AudioInputALSA());
#else
        std::cerr << "ALSA audio input is not supported on this platform."
                  << std::endl;
        return -1;
#endif
    } else {
      std::ifstream audio_file(audio_input_source);
      if (!audio_file) {
        std::cerr << "Audio input file \"" << audio_input_source
                  << "\" does not exist." << std::endl;
        return -1;
      }
      audio_input.reset(new AudioInputFile(audio_input_source));
    }

    audio_input->AddDataListener(
      [stream, &request](std::shared_ptr<std::vector<unsigned char>> data) {
        request.set_audio_in(&((*data)[0]), data->size());
        stream->Write(request);
    });
    audio_input->AddStopListener([stream]() {
      stream->WritesDone();
    });
    audio_input->Start();
  }

#ifdef ENABLE_ALSA
  AudioOutputALSA audio_output;
  audio_output.Start();
#endif

  // Read responses.
  if (verbose) {
    std::clog << "assistant_sdk waiting for response ... " << std::endl;
  }
  AssistResponse response;
  if (verbose) {
    std::clog << "assistant_sdk Got a response \n";
  }
  while (stream->Read(&response)) {  // Returns false when no more to read.
    if (response.has_audio_out() ||
        response.event_type() == AssistResponse_EventType_END_OF_UTTERANCE) {
      // Synchronously stops audio input if there is one.
      if (audio_input != nullptr && audio_input->IsRunning()) {
        audio_input->Stop();
      }
    }
    if (response.has_audio_out()) {
      // CUSTOMIZE: play back audio_out here.
#ifdef ENABLE_ALSA
      std::shared_ptr<std::vector<unsigned char>>
          data(new std::vector<unsigned char>);
      data->resize(response.audio_out().audio_data().length());
      memcpy(&((*data)[0]), response.audio_out().audio_data().c_str(),
          response.audio_out().audio_data().length());
      audio_output.Send(data);
#endif
    }
    // CUSTOMIZE: render spoken request on screen
    for (int i = 0; i < response.speech_results_size(); i++) {
      google::assistant::embedded::v1alpha2::SpeechRecognitionResult result =
          response.speech_results(i);
      if (verbose) {
        std::clog << "assistant_sdk request: \n"
                  << result.transcript() << " ("
                  << std::to_string(result.stability())
                  << ")" << std::endl;
      }
    }
    if (response.dialog_state_out().supplemental_display_text().size() > 0) {
      // CUSTOMIZE: render spoken response on screen
      std::clog << "assistant_sdk response:" << std::endl;
      std::cout << response.dialog_state_out().supplemental_display_text()
                << std::endl;
    }
  }

#ifdef ENABLE_ALSA
  audio_output.Stop();
#endif

  grpc::Status status = stream->Finish();
  if (!status.ok()) {
    // Report the RPC failure.
    std::cerr << "assistant_sdk failed, error: " <<
              status.error_message() << std::endl;
    return -1;
  }

  return 0;
}
