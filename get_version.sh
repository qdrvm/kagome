#!/bin/sh -eu
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

sanitize_version() {
  echo "$1" | sed -E 's/[^a-zA-Z0-9.+~:-]/-/g'
}

realpath() {
  if [ -d "$1" ]; then
    cd "$1" && pwd
  else
    echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
  fi
}

cd "$(dirname "$(realpath "$0")")"

SANITIZED=false
if [ "$#" -gt 0 ] && [ "$1" = "--sanitized" ]; then
  SANITIZED=true
fi

if [ -x "$(which git 2>/dev/null)" ] && [ -e ".git" ]; then
  if [ -x "$(which sed 2>/dev/null)" ]; then
    HEAD=$(git rev-parse --short HEAD)
    
    # Use git describe to get the most recent tag reachable from HEAD
    DESCR=$(git describe --tags --long)
    if [ "$DESCR" = "" ]; then
      DESCR=$HEAD-0-g$HEAD
    fi

    TAG=$(echo "$DESCR" | sed -E "s/v?(.*)-([0-9]+)-g[a-f0-9]+/\1/")
    COMMITS_SINCE_TAG=$(echo "$DESCR" | sed -E "s/v?(.*)-([0-9]+)-g[a-f0-9]+/\2/")

    BRANCH=$(git branch --show-current)
    if [ "$BRANCH" = "" ]; then
      BRANCH=$HEAD
    fi

    RESULT=$TAG
    if [ "$COMMITS_SINCE_TAG" != "0" ]; then
      RESULT=$RESULT-$COMMITS_SINCE_TAG
      if [ "$BRANCH" != "master" ]; then
        RESULT=$RESULT-$BRANCH-$COMMITS_SINCE_TAG-$HEAD
      fi
    fi
  else
    RESULT=$(git describe --tags --long HEAD)
  fi

  DIRTY=$(git diff --quiet || echo '-dirty')

  RESULT=$RESULT$DIRTY
else
  RESULT="Unknown(no git)"
fi

if [ "$SANITIZED" = true ]; then
  RESULT=$(sanitize_version "$RESULT")
fi

echo "$RESULT"