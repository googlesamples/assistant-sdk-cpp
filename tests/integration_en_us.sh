# Tests that the command "how do you say hi in spanish"
#   will return "Hola" to verify the Assistant SDK
#   end-to-end.
set -e
set -x

./run_assistant --text_input "how do you say hi in spanish" \
  --credentials_file ./credentials.json --credentials_type USER_ACCOUNT \
  --verbose | grep "Hola"
