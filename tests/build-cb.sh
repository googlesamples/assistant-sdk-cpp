#!/bin/bash
set -e
set -u
set -o pipefail
set -x
PROJECT_PATH=$(pwd)

# Step 0. Install git?
apt-get update
apt-get install -y git

# Step 1. Clean up any dependencies
echo "Cleaning dependencies"

# Copy w/o `sudo`

# Greedily cleans the project and system-wide dependencies
# This will put the development machine in a clean state
# to install development tools.
git clean -xfd -e *.json
rm -rf ./grpc/ ./googleapis/
# https://github.com/grpc/grpc/pull/10706#issuecomment-302775038
apt-get purge -y libc-ares-dev
apt-get purge -y libprotobuf-dev libprotoc-dev
# Remove generated files to prepare for full compilation
rm -rf /usr/local/bin/grpc_* \
    /usr/local/bin/protoc \
    /usr/local/include/google/protobuf/ \
    /usr/local/include/grpc/ \
    /usr/local/include/grpc++/ \
    /usr/local/lib/libproto* \
    /usr/local/lib/libgpr* \
    /usr/local/lib/libgrpc* \
    /usr/local/lib/pkgconfig/protobuf* \
    /usr/local/lib/pkgconfig/grpc* \
    /usr/local/share/grpc/

# Step 2. Install dependencies
echo "Installing dependencies"
apt-get install -y autoconf automake libtool build-essential curl unzip pkg-config
apt-get install -y libasound2-dev  # For ALSA sound output
apt-get install -y libcurl4-openssl-dev # CURL development library

# Step 3. Build protocol buffer, gRPC, and Google APIs
git clone -b "$(curl -L https://grpc.io/release)" https://github.com/grpc/grpc
GRPC_PATH=${PROJECT_PATH}/grpc
cd "${GRPC_PATH}"
# Checkout stable release of gRPC
git checkout v1.15.0
git submodule update --init

echo "Compiling gRPC protobufs"
cd third_party/protobuf
./autogen.sh && ./configure && make
make install
ldconfig

echo "Compiling gRPC"
cd "${GRPC_PATH}"
make clean
LDFLAGS="-lm" make
make install
ldconfig

echo "Compiling Google APIs"
cd "${PROJECT_PATH}"
git clone https://github.com/googleapis/googleapis.git
cd googleapis/
# Remove unnecessary directories
cd google/
find . ! -name 'embedded_assistant.proto' \
      ! -name 'annotations.proto' \
      ! -name 'latlng.proto' \
      ! -name 'http.proto' \
      ! -name 'status.proto' \
      -type f -exec rm '{}' \;
cd ../
make LANGUAGE=cpp -j16

# Step 4. Build assistant-grpc
echo "Compiling C++ Assistant"
cd "${PROJECT_PATH}"
GOOGLEAPIS_GENS_PATH=${PROJECT_PATH}/googleapis/gens make run_assistant

# If we get to this point, the project has successfully been built, and we can exit.
echo "Project built!"
