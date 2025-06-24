#!/bin/sh
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

# Runs clang-format (preferrably version 16) on 
# files that are modified according to git

if git rev-parse --verify HEAD >/dev/null 2>&1; then
  BASE=HEAD
else
  # Initial commit: diff BASE an empty tree object
  BASE=$(git hash-object -t tree /dev/null)
fi

# check clang-format binary
CLANG_FORMAT=$(which clang-format-16 2>/dev/null)
if [ -z "${CLANG_FORMAT}" ]; then
  CLANG_FORMAT=$(which clang-format)
  if [ -z "${CLANG_FORMAT}" ]; then
    echo "Command clang-format is not found" >&2
    echo "Please, install clang-format version 16 to enable checkup C++-files formatting over git pre-commit hook" >&2
    exit 1
  fi
fi

FILES=$(git diff --staged --diff-filter=ACMR --name-only)

for FILE in $(echo "$FILES" | grep -e "\\.[ch]pp$"); do
    echo "Format $FILE" 
    ${CLANG_FORMAT} -i --style=file "$FILE"
done
