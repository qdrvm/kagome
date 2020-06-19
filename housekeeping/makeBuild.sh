#!/bin/bash -xe

BUILD_DIR="${BUILD_DIR:?BUILD_DIR variable is not defined}"
BUILD_FINAL_TARGET="${BUILD_FINAL_TARGET:-test}"
BUILD_TREADS="${BUILD_TREADS:-4}"

which git

cd "$(dirname $0)/.."
git submodule update --init --recursive
cmake . -B${BUILD_DIR} "$@"
cmake --build "${BUILD_DIR}" -- -j${BUILD_TREADS}
cmake --build "${BUILD_DIR}" --target "${BUILD_FINAL_TARGET}"
