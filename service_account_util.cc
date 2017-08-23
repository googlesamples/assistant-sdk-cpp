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

#include "service_account_util.h"

#include "scope_exit.h"

extern "C" {
#include <src/core/lib/json/json.h>
#include <src/core/lib/security/credentials/jwt/json_token.h>
#include <src/core/lib/security/util/json_util.h>
}

#include <curl/curl.h>

static const int kAccessTokenExpirationTimeInSeconds = 3600;

size_t write_data(void* ptr, size_t size, size_t nmemb, void* s) {
  std::string* str = (std::string*) s;
  size_t old_length = str->length();
  str->resize(old_length + size * nmemb);
  memcpy((void*)(str->c_str() + old_length), ptr, size * nmemb);
  return size * nmemb;
}

std::shared_ptr<grpc::CallCredentials> GetServiceAccountCredentialsOrNull(
    const std::string& service_account_json) {
  grpc_auth_json_key json_key = grpc_auth_json_key_create_from_string(
      service_account_json.c_str());
  ScopeExit destroy_json_key([&json_key]() {
    grpc_auth_json_key_destruct(&json_key);
  });
  if (!grpc_auth_json_key_is_valid(&json_key)) {
    std::cerr << "Failed to parse service account json." << std::endl;
    return nullptr;
  }

  gpr_timespec token_lifetime = gpr_time_from_seconds(
      kAccessTokenExpirationTimeInSeconds, GPR_TIMESPAN);
  char* jwt = grpc_jwt_encode_and_sign(
    &json_key, "https://www.googleapis.com/oauth2/v4/token", token_lifetime,
    "https://www.googleapis.com/auth/assistant-sdk-prototype");
  if (jwt == nullptr) {
    std::cerr << "Failed to sign access token request." << std::endl;
    return nullptr;
  }
  std::string curl_data =
      R"(grant_type=urn:ietf:params:oauth:grant-type:jwt-bearer&assertion=)" + std::string(jwt);

  std::string response;
  CURL *hnd = curl_easy_init();
  if (hnd == nullptr) {
    std::cerr << "curl_easy_init() returned null" << std::endl;
    return nullptr;
  }
  curl_easy_setopt(hnd, CURLOPT_URL, "https://www.googleapis.com/oauth2/v4/token");
  curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, curl_data.c_str());
  curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &response);
  CURLcode ret = curl_easy_perform(hnd);
  ScopeExit destroy_curl([hnd]() {
    curl_easy_cleanup(hnd);
  });
  if (ret != CURLE_OK) {
    std::cerr << "curl_easy_perform() returned " << ret << std::endl;
    return nullptr;
  }
  if (response.length() == 0) {
    return nullptr;
  }

  std::unique_ptr<char[]> s(new char[response.length()]);
  memcpy(s.get(), response.c_str(), response.length());
  grpc_json* json = grpc_json_parse_string_with_len(s.get(), response.length());
  if (json == nullptr) {
    std::cerr << "Failed to parse response: \"" << response << "\"" << std::endl;
    return nullptr;
  }
  ScopeExit destroy_json([json]() {
    grpc_json_destroy(json);
  });
  const char* access_token = grpc_json_get_string_property(json, "access_token");
  if (access_token == nullptr) {
    std::cerr << "No access_token in response: \"" << response << "\"" << std::endl;
    return nullptr;
  }
  return grpc::AccessTokenCredentials(std::string(access_token));
}
