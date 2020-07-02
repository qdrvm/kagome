#!/bin/bash -xe

BUILD_DIR="${BUILD_DIR:?BUILD_DIR variable is not defined}"
BUILD_FINAL_TARGET="${BUILD_FINAL_TARGET:-test}"
BUILD_TREADS="${BUILD_TREADS:-$(( $(nproc 2>/dev/null || sysctl -n hw.ncpu) + 1 ))}"

which git

cd "$(dirname $0)/.."

cmake . -B${BUILD_DIR} "$@"
if [ "$BUILD_FINAL_TARGET" == "test" ] ; then
  cmake --build "${BUILD_DIR}" -- -j${BUILD_TREADS}
fi
cmake --build "${BUILD_DIR}" --target "${BUILD_FINAL_TARGET}"
