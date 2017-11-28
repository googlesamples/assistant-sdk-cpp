# gRPC C++ samples for Google Assistant SDK

## Requirements

This project is officially supported on Ubuntu 14.04. Other Linux distributions may be able to run
this sample.

Refer to the [Assistant SDK documentation](https://developers.google.com/assistant/sdk/) for more information.

## Setup instructions

### Clean Project

_If you have not setup this project before, you can skip this step._

```
sudo apt-get purge libc-ares-dev  # https://github.com/grpc/grpc/pull/10706#issuecomment-302775038
sudo apt-get purge libprotobuf-dev libprotoc-dev
sudo rm -rf /usr/local/bin/grpc_* /usr/local/bin/protoc \
    /usr/local/include/google/protobuf/ /usr/local/include/grpc/ /usr/local/include/grpc++/ \
    /usr/local/lib/libproto* /usr/local/lib/libgpr* /usr/local/lib/libgrpc* \
    /usr/local/lib/pkgconfig/protobuf* /usr/local/lib/pkgconfig/grpc* \
    /usr/local/share/grpc/
```

### Build Project

1. Clone this project
```
git clone https://github.com/googlesamples/assistant-sdk-cpp.git
cd assistant-sdk-cpp
export PROJECT_PATH=$(pwd)
```

2. Install dependencies
```
sudo apt-get install autoconf automake libtool build-essential curl unzip
sudo apt-get install libasound2-dev  # For ALSA sound output
sudo apt-get install libcurl4-openssl-dev # CURL development library
```

3. Build Protocol Buffer, gRPC, and Google APIs
```
git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc
GRPC_PATH=${PROJECT_PATH}/grpc
cd ${GRPC_PATH}

git submodule update --init

cd third_party/protobuf
./autogen.sh && ./configure && make
sudo make install
sudo ldconfig

export LDFLAGS="$LDFLAGS -lm"
cd ${GRPC_PATH}
make clean
make
sudo make install
sudo ldconfig

cd ${PROJECT_PATH}
git clone https://github.com/googleapis/googleapis.git
cd googleapis/
make LANGUAGE=cpp
```

4. Make sure you setup environment variable `$GOOGLEAPIS_GENS_PATH`
```
export GOOGLEAPIS_GENS_PATH=${PROJECT_PATH}/googleapis/gens
```

5. Build this project
```
cd ${PROJECT_PATH}
make run_assistant
```

6. Get credentials file. It can be either an end-user's credentials, or a service account's credentials.

For end-user's credentials:

* Download the client secret json file from Google Cloud Platform Console following [these instructions](https://developers.google.com/assistant/sdk/develop/python/config-dev-project-and-account)
* Move it in this folder and rename it to `client_secret.json`
* run `get_credentials.sh` in this folder. It will create the file `credentials.json`.

7. Start `run_assistant`
```
./run_assistant --audio_input ./resources/switch_to_channel_5.raw --credentials_file ./credentials.json --credentials_type USER_ACCOUNT
```

On Linux workstation, you can alternatively use ALSA audio input:
```
./run_assistant --audio_input ALSA_INPUT --credentials_file ./credentials.json --credentials_type USER_ACCOUNT
```

Default Assistant gRPC API endpoint is embeddedassistant.googleapis.com. If you want to test with a custom Assistant gRPC API endpoint, you can pass an extra "--api_endpoint CUSTOM_API_ENDPOINT" to run_assistant.
