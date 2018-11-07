#!/bin/bash
set -e
set -u
set -o pipefail
set -x

# Tests that the command "how do you say hello in spanish"
#   will return "buenos dias" to verify the Assistant SDK
#   end-to-end.
echo "comment dit-on bonjour en español" | ./run_assistant_text \
  --credentials ./credentials.json \
  --locale "fr-FR" --verbose | grep "Buenos dias"
