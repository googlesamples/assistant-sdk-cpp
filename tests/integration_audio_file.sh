# Tests that the command "weather in mountain view"
#   will return some data including the name of the town
#   to verify the Assistant SDK end-to-end.
set -e
set -x

./run_assistant_file --input ./resources/weather_in_mountain_view.raw \
  --output /tmp/google-assistant-sdk-audio-output.raw \
  --credentials ./credentials.json \
  | grep "Mountain View"
