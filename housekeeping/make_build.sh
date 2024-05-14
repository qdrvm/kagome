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
else
  BUILD_THREADS="${BUILD_THREADS:-$(( $(nproc 2>/dev/null || sysctl -n hw.ncpu) ))}"
  git config --global --add safe.directory /__w/kagome/kagome
fi

if [[ "${KAGOME_IN_DOCKER}" = 1 ]]; then
  source /venv/bin/activate
fi

if [[ "${KAGOME_MAC_CI}" = 1 ]]; then
  source ~/venv/bin/activate
  export HUNTER_PYTHON_LOCATION=$VIRTUAL_ENV
  export CURL_SSL_BACKEND=SecureTransport
  export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
fi

which git

cd "$(dirname $0)/.."

cmake . -B${BUILD_DIR} "$@" -DBACKWARD=OFF
if [ "$BUILD_FINAL_TARGET" != "generated" ] ; then
  cmake --build "${BUILD_DIR}" --target pvf_test -- -j${BUILD_THREADS}
fi
#cmake --build "${BUILD_DIR}" --target "${BUILD_FINAL_TARGET}"
cd "${BUILD_DIR}" && ctest -R pvf_test