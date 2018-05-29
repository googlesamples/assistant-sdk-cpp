# Run all tests
# Expected usage:
#   > cpp$ ./tests/test_all.sh
set -e
set -x

# Verify it builds
./tests/build.sh

# After building, run end-to-end tests
./tests/integration_en_us.sh
./tests/integration_fr.sh
./tests/integration_audio_file.sh
