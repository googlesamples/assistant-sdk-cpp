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
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <thread>

#include "google/assistant/embedded/v1alpha2/embedded_assistant.grpc.pb.h"
#include "google/assistant/embedded/v1alpha2/embedded_assistant.pb.h"

#include "assistant_config.h"
#include "audio_input.h"
#include "audio_input_file.h"
#include "base64_encode.h"

namespace assistant = google::assistant::embedded::v1alpha2;

using assistant::AssistRequest;
using assistant::AssistResponse;
using assistant::AssistResponse_EventType_END_OF_UTTERANCE;
using assistant::AudioInConfig;
using assistant::AudioOutConfig;
using assistant::EmbeddedAssistant;
using assistant::ScreenOutConfig;

using grpc::CallCredentials;
using grpc::Channel;
using grpc::ClientReaderWriter;

static const std::string kCredentialsTypeUserAccount = "USER_ACCOUNT";
static const std::string kLanguageCode = "en-US";
static const std::string kDeviceModelId = "default";
static const std::string kDeviceInstanceId = "default";

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
    std::clog << "assistant_sdk CreateCustomChannel(" << server
              << ", creds, arg)" << std::endl
              << std::endl;
  }
  ::grpc::ChannelArguments channel_args;
  return CreateCustomChannel(server, creds, channel_args);
}

void PrintUsage() {
  std::cerr << "Usage: ./run_assistant_text "
            << "--credentials <credentials_file> "
            << "[--api_endpoint <API endpoint>] "
            << "[--locale <locale>]"
            << "[--html_out <command to load HTML file>]" << std::endl;
}

bool GetCommandLineFlags(int argc, char** argv,
                         std::string* credentials_file_path,
                         std::string* api_endpoint, std::string* locale,
                         std::string* html_out_command) {
  const struct option long_options[] = {
      {"credentials", required_argument, nullptr, 'c'},
      {"api_endpoint", required_argument, nullptr, 'e'},
      {"locale", required_argument, nullptr, 'l'},
      {"verbose", no_argument, nullptr, 'v'},
      {"html_out", required_argument, nullptr, 'h'},
      {nullptr, 0, nullptr, 0}};
  *api_endpoint = ASSISTANT_ENDPOINT;
  while (true) {
    int option_index;
    int option_char =
        getopt_long(argc, argv, "c:e:l:v:h", long_options, &option_index);
    if (option_char == -1) {
      break;
    }
    switch (option_char) {
      case 'c':
        *credentials_file_path = optarg;
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
      case 'h':
        *html_out_command = optarg;
        break;
      default:
        PrintUsage();
        return false;
    }
  }
  return true;
}

int main(int argc, char** argv) {
  std::string credentials_file_path, api_endpoint, locale, text_input_source,
      html_out_command;
  // Initialize gRPC and DNS resolvers
  // https://github.com/grpc/grpc/issues/11366#issuecomment-328595941
  grpc_init();
  if (!GetCommandLineFlags(argc, argv, &credentials_file_path, &api_endpoint,
                           &locale, &html_out_command)) {
    return -1;
  }

  while (std::getline(std::cin, text_input_source)) {
    // Create an AssistRequest
    AssistRequest request;
    auto* assist_config = request.mutable_config();

    if (locale.empty()) {
      locale = kLanguageCode;  // Default locale
    }
    if (verbose) {
      std::clog << "Using locale " << locale << std::endl;
    }
    // Set the DialogStateIn of the AssistRequest
    assist_config->mutable_dialog_state_in()->set_language_code(locale);

    // Set the DeviceConfig of the AssistRequest
    assist_config->mutable_device_config()->set_device_id(kDeviceInstanceId);
    assist_config->mutable_device_config()->set_device_model_id(kDeviceModelId);

    // Set parameters for audio output
    assist_config->mutable_audio_out_config()->set_encoding(
        AudioOutConfig::LINEAR16);
    assist_config->mutable_audio_out_config()->set_sample_rate_hertz(16000);
    assist_config->set_text_query(text_input_source);

    // Set parameters for screen config
    assist_config->mutable_screen_out_config()->set_screen_mode(
        html_out_command.empty() ? ScreenOutConfig::SCREEN_MODE_UNSPECIFIED
                                 : ScreenOutConfig::PLAYING);

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

    std::shared_ptr<ClientReaderWriter<AssistRequest, AssistResponse>> stream(
        std::move(assistant->Assist(&context)));
    // Write config in first stream.
    if (verbose) {
      std::clog << "assistant_sdk wrote first request: "
                << request.ShortDebugString() << std::endl;
    }
    stream->Write(request);

    // Read responses.
    if (verbose) {
      std::clog << "assistant_sdk waiting for response ... " << std::endl;
    }
    AssistResponse response;
    while (stream->Read(&response)) {  // Returns false when no more to read.
      // CUSTOMIZE: render spoken request on screen
      for (int i = 0; i < response.speech_results_size(); i++) {
        auto result = response.speech_results(i);
        if (verbose) {
          std::clog << "assistant_sdk request: \n"
                    << result.transcript() << " ("
                    << std::to_string(result.stability()) << ")" << std::endl;
        }
      }
      if (!html_out_command.empty() &&
          response.screen_out().data().size() > 0) {
        std::string html_out_base64 =
            base64_encode(response.screen_out().data());
        system((html_out_command + " \"data:text/html;base64, " +
                html_out_base64 + "\"")
                   .c_str());
      } else if (html_out_command.empty()) {
        if (response.dialog_state_out().supplemental_display_text().size() >
            0) {
          // CUSTOMIZE: render spoken response on screen
          std::clog << "assistant_sdk response:" << std::endl;
          std::cout << response.dialog_state_out().supplemental_display_text()
                    << std::endl;
        }
      }
    }

    grpc::Status status = stream->Finish();
    if (!status.ok()) {
      // Report the RPC failure.
      std::cerr << "assistant_sdk failed, error: " << status.error_message()
                << std::endl;
      return -1;
    }
  }

  return 0;
}
