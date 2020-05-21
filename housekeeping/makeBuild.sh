#!/bin/bash -xe

BUILD_DIR="${BUILD_DIR:?BUILD_DIR variable is not defined}"
BUILD_TARGET="${BUILD_TARGET:-test}"
BUILD_TREADS="${BUILD_TREADS:-4}"

which git

cd "$(dirname $0)/.."

cmake . -B${BUILD_DIR} "$@"
cmake --build "${BUILD_DIR}" -- -j${BUILD_TREADS}
cmake --build "${BUILD_DIR}" --target "${BUILD_TARGET}"
