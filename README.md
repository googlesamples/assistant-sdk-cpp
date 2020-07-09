# Google Assistant SDK for devices - C++

### Linux / OSX build status
[![Build Status](https://travis-ci.com/jwinarske/assistant-sdk-cpp.svg?branch=cmake)](https://travis-ci.com/jwinarske/assistant-sdk-cpp)

### Windows build status
[![Build status](https://ci.appveyor.com/api/projects/status/op7vmksh3p0jkln4/branch/cmake?svg=true)](https://ci.appveyor.com/project/jwinarske/assistant-sdk-cpp/branch/cmake)

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
```

2. Install dependencies
```
sudo apt-get install autoconf automake libtool build-essential curl unzip
sudo apt-get install libasound2-dev  # For ALSA sound output
sudo apt-get install libcurl4-openssl-dev # CURL development library
```

3. Build this project
```
mkdir build && cd build
cmake .. -DCMAKE_STAGING_PREFIX=`pwd`/dist/usr/local
make install -j
```

4. Get credentials file. It must be an end-user's credentials.

* Go to the [Actions Console](https://console.actions.google.com/) and register your device model, following [these instructions](https://developers.google.com/assistant/sdk/guides/library/python/embed/register-device)
* Move it in this folder and rename it to `client_secret.json`
* run `get_credentials.sh` in this folder. It will create the file `credentials.json`.

5. Start one of the `run_assistant` samples:

```bash
export LD_LIBRARY_PATH=`pwd`/dist/usr/local/lib
./dist/usr/local/bin/run_assistant_file --input ../resources/weather_in_mountain_view.raw --output ./response.wav --credentials ../credentials.json
aplay ./response.wav --rate=16000 --format=S16_LE
```

On a Linux workstation, you can alternatively use ALSA for audio input:

```bash
export LD_LIBRARY_PATH=`pwd`/dist/usr/local/lib
./dist/usr/local/bin/run_assistant_audio --credentials ../credentials.json
```

You can use a text-based query instead of audio. This allows you to continually enter text queries to the Assistant.

```bash
export LD_LIBRARY_PATH=`pwd`/dist/usr/local/lib
./dist/usr/local/bin/run_assistant_text --credentials ../credentials.json
```

This takes input from `cin`, so you can send data to the program when it starts.

```bash
echo "what time is it" | ./dist/usr/local/bin/run_assistant_text --credentials ../credentials.json
```

To change the locale, include a `locale` parameter:

```bash
echo "Bonjour" | ./dist/usr/local/bin/run_assistant_text --credentials ../credentials.json --locale "fr-FR"
```

Default Assistant gRPC API endpoint is `embeddedassistant.googleapis.com`. If you want to test with a custom Assistant gRPC API endpoint, you can pass `--api_endpoint CUSTOM_API_ENDPOINT`.

## Enabling screen output

To get a visual output from the Assistant, provide a command to be run alongside every step of the conversation. It will execute that command along along with a provided argument of a temporary HTML file.

```bash
echo "what time is it" | ./dist/usr/local/bin/run_assistant_text --credentials ../credentials.json --html_out google-chrome
```

After you enter text, it will run `google-chrome /tmp/google-assistant-cpp-screen-out.html`.

If you prefer a different program, use that argument instead.
