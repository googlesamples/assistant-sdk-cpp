#!/bin/bash
set -e
set -u
set -o pipefail
set -x

# Tests that the command "How do you say hi in spanish?"
#   will return "Hola" to verify the Assistant SDK
#   end-to-end.
echo "How do you say hi in spanish?" | ./run_assistant_text \
  --credentials ./credentials.json --verbose | grep "Hola"
