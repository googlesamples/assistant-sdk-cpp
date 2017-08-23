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
GRPC_SRC_PATH ?= ../grpc
GOOGLEAPIS_GENS_PATH ?= ../googleapis/gens
GOOGLEAPIS_API_CCS = $(shell find $(GOOGLEAPIS_GENS_PATH)/google/api \
	-name '*.pb.cc')
GOOGLEAPIS_RPC_CCS = $(shell find $(GOOGLEAPIS_GENS_PATH)/google/rpc \
	-name '*.pb.cc')

GOOGLEAPIS_CCS = $(GOOGLEAPIS_API_CCS) $(GOOGLEAPIS_RPC_CCS)

GOOGLEAPIS_ASSISTANT_CCS = embedded_assistant.pb.cc embedded_assistant.grpc.pb.cc

HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CXX = g++
CPPFLAGS += -I/usr/local/include -pthread -I$(GOOGLEAPIS_GENS_PATH) \
	    -I$(GRPC_SRC_PATH)
CXXFLAGS += -std=c++11
# grpc_cronet is for JSON functions in gRPC library.
ifeq ($(SYSTEM),Darwin)
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -lgrpc++_reflection -lgrpc_cronet                      \
           -lprotobuf -lpthread -ldl -lcurl
else
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -lgrpc_cronet -Wl,--no-as-needed -lgrpc++_reflection   \
           -Wl,--as-needed -lprotobuf -lpthread -ldl -lcurl
endif

AUDIO_SRCS =
ifeq ($(SYSTEM),Linux)
AUDIO_SRCS += audio_input_alsa.cc audio_output_alsa.cc
LDFLAGS += `pkg-config --libs alsa`
endif

.PHONY: all
all: run_assistant

googleapis.ar: $(GOOGLEAPIS_CCS:.cc=.o)
	ar r $@ $?

run_assistant.o: $(GOOGLEAPIS_ASSISTANT_CCS:.cc=.h)

run_assistant: run_assistant.o $(GOOGLEAPIS_ASSISTANT_CCS:.cc=.o) googleapis.ar \
	$(AUDIO_SRCS:.cc=.o) audio_input_file.o json_util.o service_account_util.o
	$(CXX) $^ $(LDFLAGS) -o $@

json_util_test: json_util.o json_util_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

$(GOOGLEAPIS_ASSISTANT_CCS:.cc=.h) $(GOOGLEAPIS_ASSISTANT_CCS): embedded_assistant.proto
	protoc --proto_path=.:$(GOOGLEAPIS_GENS_PATH)/..:/usr/local/include \
	--cpp_out=./ --grpc_out=./ --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin $^

clean:
	rm -f *.o run_assistant googleapis.ar \
		$(GOOGLEAPIS_CCS:.cc=.o) \
		$(GOOGLEAPIS_ASSISTANT_CCS) $(GOOGLEAPIS_ASSISTANT_CCS:.cc=.h) \
		$(GOOGLEAPIS_ASSISTANT_CCS:.cc=.o)
