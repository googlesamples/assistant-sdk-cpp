#!/bin/bash
set -e
set -u
set -o pipefail
set -x

# Tests that the command "How do you say hello in spanish?"
#   will return "Buenos dias" to verify the Assistant SDK
#   end-to-end.
echo "Comment dit-on bonjour en espagnol?" | ./run_assistant_text \
  --credentials ./credentials.json \
  --locale "fr-FR" --verbose | grep "Buenos dias"
