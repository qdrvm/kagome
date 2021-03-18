#!/bin/bash -xe

BUILD_DIR=build
BUILD_THREADS="${BUILD_THREADS:-$(( $(nproc 2>/dev/null || sysctl -n hw.ncpu) + 1 ))}"

cd "$(dirname $0)/../../.."

cmake . -B${BUILD_DIR} -G 'Ninja' -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --target kagome_full_syncing kagome_validating -- -j${BUILD_THREADS}
