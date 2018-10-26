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

#include "assistant/json_util.h"

#include <iostream>
#include <utility>

bool check_result(const std::string& input_json,
                  std::unique_ptr<std::string> intended_result) {
  std::unique_ptr<std::string> result = GetCustomResponseOrNull(input_json);
  return (intended_result == nullptr && result == nullptr) ||
         (intended_result != nullptr && result != nullptr &&
          *intended_result == *result);
}

int main() {
  std::string invalid_json = "";
  std::unique_ptr<std::string> intended_result(nullptr);
  if (!check_result(invalid_json, std::move(intended_result))) {
    std::cerr << "Test failed for invalid JSON" << std::endl;
    return 1;
  }

  std::string incomplete_json = "{}";
  intended_result.reset(nullptr);
  if (!check_result(incomplete_json, std::move(intended_result))) {
    std::cerr << "Test failed for incomplete JSON" << std::endl;
    return 1;
  }

  std::cerr << "Test passed" << std::endl;
}
