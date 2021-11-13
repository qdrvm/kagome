#!/usr/bin/env bash

set -euo pipefail

function realpath {
  if [[ -d "$1" ]]; then
    cd "$1" && pwd
  else
    echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
  fi
}

SCRIPT_DIR=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

echo "Script is in: $SCRIPT_DIR"

cd "$SCRIPT_DIR"

KAGOME_DIR="$SCRIPT_DIR/../.."

echo "Kagome is in: $(realpath "$KAGOME_DIR")"

cd "$KAGOME_DIR"

KAGOME_BINARY_DIR="$SCRIPT_DIR/kagome_build"
KAGOME_INSTALL_DIR="$SCRIPT_DIR/kagome_install"

mkdir -p "$KAGOME_BINARY_DIR"
mkdir -p "$KAGOME_INSTALL_DIR"

cmake -B "$KAGOME_BINARY_DIR" -DCMAKE_INSTALL_PREFIX="$KAGOME_INSTALL_DIR" -DTESTING=OFF "$@"
BUILD_THREADS="${BUILD_THREADS:-$(( $(nproc 2>/dev/null || sysctl -n hw.ncpu) + 1 ))}"
cmake --build "$KAGOME_BINARY_DIR" -- install -j${BUILD_THREADS}
