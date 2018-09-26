# Copyright 2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
GRPC_SRC_PATH ?= ./grpc
GOOGLEAPIS_GENS_PATH ?= ./googleapis/gens
GOOGLEAPIS_API_CCS = $(shell find $(GOOGLEAPIS_GENS_PATH)/google/api \
	-name '*.pb.cc')
GOOGLEAPIS_ASSISTANT_CCS = $(shell find $(GOOGLEAPIS_GENS_PATH)/google/assistant/embedded/v1alpha2 \
	-name '*.pb.cc')
GOOGLEAPIS_TYPE_CCS = $(shell find $(GOOGLEAPIS_GENS_PATH)/google/type \
	-name '*.pb.cc')
GOOGLEAPIS_RPC_CCS = $(shell find $(GOOGLEAPIS_GENS_PATH)/google/rpc \
	-name '*.pb.cc')

GOOGLEAPIS_CCS = $(GOOGLEAPIS_API_CCS) $(GOOGLEAPIS_RPC_CCS) $(GOOGLEAPIS_TYPE_CCS)

GOOGLEAPIS_ASSISTANT_CCS = ./googleapis/gens/google/assistant/embedded/v1alpha2/embedded_assistant.pb.cc \
	./googleapis/gens/google/assistant/embedded/v1alpha2/embedded_assistant.grpc.pb.cc

HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CXX = g++
CPPFLAGS += -I/usr/local/include -pthread -I$(GOOGLEAPIS_GENS_PATH) \
	    -I$(GRPC_SRC_PATH) -I./src/
CXXFLAGS += -std=c++11
# grpc_cronet is for JSON functions in gRPC library.
ifeq ($(SYSTEM),Darwin)
LDFLAGS += $(GRPC_GRPCPP_LDLAGS) \
           -lgrpc++_reflection -lgrpc_cronet \
           -lprotobuf -lpthread -ldl
else
LDFLAGS += $(GRPC_GRPCPP_LDLAGS) \
           -lgrpc_cronet -Wl,--no-as-needed -lgrpc++_reflection \
           -Wl,--as-needed -lprotobuf -lpthread -ldl
endif

AUDIO_SRCS =
ifeq ($(SYSTEM),Linux)
AUDIO_SRCS += src/audio_input_alsa.cc src/audio_output_alsa.cc
LDFLAGS += `pkg-config --libs alsa`
endif

CORE_SRCS = ./src/base64_encode.cc ./src/json_util.cc
AUDIO_INPUT_FILE_SRCS = ./src/audio_input_file.cc
ASSISTANT_AUDIO_SRCS = ./src/run_assistant_audio.cc
ASSISTANT_FILE_SRCS = ./src/run_assistant_file.cc
ASSISTANT_TEXT_SRCS = ./src/run_assistant_text.cc

ASSISTANT_O       = $(CORE_SRCS:.cc=.o) \
                    $(AUDIO_SRCS:.cc=.o) \
                    $(AUDIO_INPUT_FILE_SRCS:.cc=.o) \
                    $(ASSISTANT_AUDIO_SRCS:.cc=.o) \
                    $(ASSISTANT_FILE_SRCS:.cc=.o) \
                    $(ASSISTANT_TEXT_SRCS:.cc=.o)
ASSISTANT_AUDIO_O = $(CORE_SRCS:.cc=.o) \
                    $(AUDIO_SRCS:.cc=.o) \
                    $(AUDIO_INPUT_FILE_SRCS:.cc=.o) \
                    $(ASSISTANT_AUDIO_SRCS:.cc=.o)
ASSISTANT_FILE_O  = $(CORE_SRCS:.cc=.o) \
                    $(AUDIO_INPUT_FILE_SRCS:.cc=.o) \
                    $(ASSISTANT_FILE_SRCS:.cc=.o)
ASSISTANT_TEXT_O  = $(CORE_SRCS:.cc=.o) \
                    $(ASSISTANT_TEXT_SRCS:.cc=.o)

.PHONY: all
all: run_assistant

googleapis.ar: $(GOOGLEAPIS_CCS:.cc=.o)
	ar r $@ $?

.PHONY: run_assistant
run_assistant: run_assistant_audio run_assistant_file run_assistant_text

run_assistant_audio: $(GOOGLEAPIS_ASSISTANT_CCS:.cc=.o) googleapis.ar \
	$(ASSISTANT_AUDIO_O)
	$(CXX) $^ $(LDFLAGS) -o $@

run_assistant_file: $(GOOGLEAPIS_ASSISTANT_CCS:.cc=.o) googleapis.ar \
	$(ASSISTANT_FILE_O)
	$(CXX) $^ $(LDFLAGS) -o $@

run_assistant_text: $(GOOGLEAPIS_ASSISTANT_CCS:.cc=.o) googleapis.ar \
	$(ASSISTANT_TEXT_O)
	$(CXX) $^ $(LDFLAGS) -o $@

json_util_test: ./src/json_util.o ./src/json_util_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

$(GOOGLEAPIS_ASSISTANT_CCS:.cc=.h) $(GOOGLEAPIS_ASSISTANT_CCS):
	protoc -I=$(PROTO_PATH) --proto_path=.:$(GOOGLEAPIS_GENS_PATH)/..:/usr/local/include \
	--cpp_out=./src --grpc_out=./src --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin $(PROTO_PATH)/embedded_assistant.proto $^

protobufs: $(GOOGLEAPIS_ASSISTANT_CCS:.cc=.h) $(GOOGLEAPIS_ASSISTANT_CCS)

clean:
	rm -f run_assistant_text run_assistant_audio run_assistant_file googleapis.ar \
		$(GOOGLEAPIS_CCS:.cc=.o) \
		$(GOOGLEAPIS_ASSISTANT_CCS) $(GOOGLEAPIS_ASSISTANT_CCS:.cc=.h) \
		$(GOOGLEAPIS_ASSISTANT_CCS:.cc=.o) \
		$(ASSISTANT_O)
