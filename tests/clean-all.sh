# Greedily cleans the project and system-wide dependencies
# This will put the development machine in a clean state
# to install development tools.
rm -f *.o run_assistant googleapis.ar ./src/*.o
rm -rf ./grpc/ ./googleapis/
# https://github.com/grpc/grpc/pull/10706#issuecomment-302775038
sudo apt-get purge -y libc-ares-dev
sudo apt-get purge -y libprotobuf-dev libprotoc-dev
# Remove generated files to prepare for full compilation
sudo rm -rf /usr/local/bin/grpc_* \
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