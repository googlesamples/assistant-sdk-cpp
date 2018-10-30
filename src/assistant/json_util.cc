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

extern "C" {
#include <src/core/lib/json/json.h>
}

#include <cstring>
#include <iostream>

#include "assistant/scope_exit.h"

grpc_json* GetJsonValueOrNullFromDict(grpc_json* dict_node, const char* key) {
  if (dict_node->type != GRPC_JSON_OBJECT) {
    return nullptr;
  }

  grpc_json* child = dict_node->child;
  while (child != nullptr) {
    if (child->key != nullptr && strcmp(child->key, key) == 0) {
      return child;
    }
    child = child->next;
  }
  return nullptr;
}

grpc_json* GetJsonValueOrNullFromArray(grpc_json* array_node, int index) {
  if (array_node->type != GRPC_JSON_ARRAY) {
    return nullptr;
  }

  grpc_json* child = array_node->child;
  while (child != nullptr && index != 0) {
    child = child->next;
    index--;
  }
  return child;
}
