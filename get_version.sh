#!/bin/sh -eu
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

realpath() {
  if [ -d "$1" ]; then
    cd "$1" && pwd
  else
    echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
  fi
}

cd $(dirname "$(realpath "$0")")

if [ -x "$(which git 2>/dev/null)" ] && [ -e ".git" ]; then
  if [ -x "$(which sed 2>/dev/null)" ]; then
    HEAD=$(git rev-parse --short HEAD)
    COMMON=$(git merge-base HEAD master)

    DESCR=$(git describe --tags --long ${COMMON})
    if [ "$DESCR" = "" ]; then
      DESCR=$HEAD-0-g$HEAD
    fi

    TAG_IN_MASTER=$(echo $DESCR | sed -E "s/v?(.*)-([0-9]+)-g[a-f0-9]+/\1/")
    TAG_TO_FORK_DISTANCE=$(echo $DESCR | sed -E "s/v?(.*)-([0-9]+)-g[a-f0-9]+/\2/")

    BRANCH=$(git branch --show-current)
    if [ "$BRANCH" = "" ]; then
      BRANCH=$HEAD
    fi
    FORK_TO_HEAD_DISTANCE=$(git rev-list --count ${COMMON}..HEAD)

    RESULT=$TAG_IN_MASTER
    if [ "$TAG_TO_FORK_DISTANCE" != "0" ]; then
      RESULT=$RESULT-$TAG_TO_FORK_DISTANCE
      if [ "$BRANCH" != "master" ]; then
        RESULT=$RESULT-$BRANCH-$FORK_TO_HEAD_DISTANCE-$HEAD
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

echo $RESULT
