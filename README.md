# Google Assistant SDK for devices - C++

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
sudo apt-get install autoconf automake libtool build-essential curl unzip pkg-config
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
git checkout 980cdfa
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

6. Get credentials file. It must be an end-user's credentials.

* Go to the [Actions Console](https://console.actions.google.com/) and register your device model, following [these instructions](https://developers.google.com/assistant/sdk/guides/library/python/embed/register-device)
* Move it in this folder and rename it to `client_secret.json`
* run `get_credentials.sh` in this folder. It will create the file `credentials.json`.

7. Start one of the `run_assistant` samples:

```bash
./run_assistant_file --input ./resources/weather_in_mountain_view.raw --output ./response.wav --credentials ./credentials.json
aplay ./response.wav --rate=16000 --format=S16_LE
```

On a Linux workstation, you can alternatively use ALSA for audio input:

```bash
./run_assistant_audio --credentials ./credentials.json
```

You can use a text-based query instead of audio. This allows you to continually enter text queries to the Assistant.

```bash
./run_assistant_text --credentials ./credentials.json
```

This takes input from `cin`, so you can send data to the program when it starts.

```bash
echo "what time is it" | ./run_assistant_text --credentials ./credentials.json
```

To change the locale, include a `locale` parameter:

```bash
echo "Bonjour" | ./run_assistant_text --credentials ./credentials.json --locale "fr-FR"
```

Default Assistant gRPC API endpoint is `embeddedassistant.googleapis.com`. If you want to test with a custom Assistant gRPC API endpoint, you can pass `--api_endpoint CUSTOM_API_ENDPOINT`.

## Enabling screen output

To get a visual output from the Assistant, provide a command to be run alongside every step of the conversation. It will execute that command along along with a provided argument of a temporary HTML file.

```bash
echo "what time is it" | ./run_assistant_text --credentials ./credentials.json --html_out google-chrome
```

After you enter text, it will run `google-chrome /tmp/google-assistant-cpp-screen-out.html`.

If you prefer a different program, use that argument instead.
