#!/bin/bash
set -e
set -u
set -o pipefail
set -x

LINTER="cpplint.py"
if ! [ -x "$(command -v ${LINTER})" ]; then
  echo "Error: ${LINTER} not found. Add it to the PATH or get it first at https://github.com/google/styleguide" >&2
  exit 1
fi

find "$(git rev-parse --show-toplevel)/src" -type f \( -iname \*.h -o -iname \*.cc \) -print -exec ${LINTER} --quiet {} \;
