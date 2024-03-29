#!/usr/bin/env bash
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

set -xeuo pipefail

function realpath {
  if [[ -d "$1" ]]; then
    cd "$1" && pwd
  else
    echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
  fi
}

SCRIPT_DIR=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

cd "$SCRIPT_DIR"

KAGOME_DIR="$(realpath "$SCRIPT_DIR/..")"
EXTERNAL_PROJECT_DIR="$(realpath "$KAGOME_DIR/test/external-project-test")"

cd "$EXTERNAL_PROJECT_DIR"

BUILD_DIR="${BUILD_DIR:?BUILD_DIR variable is not defined}"
BUILD_TARGET="${BUILD_FINAL_TARGET:-main}"

EXTERNAL_PROJECT_BINARY_DIR="$BUILD_DIR"

mkdir -p "$EXTERNAL_PROJECT_BINARY_DIR"

if [[ -z "${CI}" ]]; then
  BUILD_THREADS="${BUILD_THREADS:-$(( $(nproc 2>/dev/null || sysctl -n hw.ncpu) / 2 + 1 ))}"
else
  BUILD_THREADS="${BUILD_THREADS:-$(( $(nproc 2>/dev/null || sysctl -n hw.ncpu) ))}"
  git config --global --add safe.directory /__w/kagome/kagome
fi

if [[ "${KAGOME_IN_DOCKER}" = 1 ]]; then
  source /venv/bin/activate
fi

cmake -B "$EXTERNAL_PROJECT_BINARY_DIR" "$@" -DBACKWARD=OFF

cmake --build "$EXTERNAL_PROJECT_BINARY_DIR" --target "${BUILD_TARGET}" -- -j${BUILD_THREADS}

"$EXTERNAL_PROJECT_BINARY_DIR"/main
