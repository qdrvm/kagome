#!/bin/sh

GIT=$(which git)

if [ -z "${GIT}" ]; then
  echo "Command ``git'' command not found"
  exit 1
fi

cd "$(dirname "$0")/.." || (echo "Run script from file" | exit 1)

${GIT} config --local core.hooksPath .githooks || (echo "Hooks activation has failed" | exit 1)

echo "Hooks activated successfully"
exit 0
