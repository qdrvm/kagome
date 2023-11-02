#!/bin/bash -xe
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

BUILD_DIR="${BUILD_DIR:?BUILD_DIR variable is not defined}"
BUILD_FINAL_TARGET="${BUILD_FINAL_TARGET:-test}"

if [[ -z "${CI}" ]]; then
  BUILD_THREADS="${BUILD_THREADS:-$(( $(nproc 2>/dev/null || sysctl -n hw.ncpu) / 2 + 1 ))}"
else # CI
  BUILD_THREADS="${BUILD_THREADS:-$(( $(nproc 2>/dev/null || sysctl -n hw.ncpu) ))}"
  git config --global --add safe.directory /__w/kagome/kagome
  source /venv/bin/activate
fi

which git

cd "$(dirname $0)/.."

cmake . -B${BUILD_DIR} "$@" -DBACKWARD=OFF
if [ "$BUILD_FINAL_TARGET" != "generated" ] ; then
  cmake --build "${BUILD_DIR}" -- -j${BUILD_THREADS}
fi
cmake --build "${BUILD_DIR}" --target "${BUILD_FINAL_TARGET}"
