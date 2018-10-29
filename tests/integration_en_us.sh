#!/bin/bash
set -e
set -u
set -o pipefail
set -x

# Tests that the command "how do you say hi in spanish"
#   will return "Hola" to verify the Assistant SDK
#   end-to-end.
echo "how do you say hi in spanish" | ./run_assistant_text \
  --credentials ./credentials.json --verbose | grep "Hola"
