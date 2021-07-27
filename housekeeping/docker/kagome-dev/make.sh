#!/bin/bash -xe

BUILD_DIR=build
BUILD_THREADS="${BUILD_THREADS:-$(( $(nproc 2>/dev/null || sysctl -n hw.ncpu) + 1 ))}"

BUILD_TYPE="${BUILD_TYPE:?BUILD_TYPE variable is not defined}"

if [ "$BUILD_TYPE" != "Debug" ] && [ "$BUILD_TYPE" != "Release" ]; then
  echo "Invalid build type $BUILD_TYPE, should be either Debug or Release"
fi

git submodule update --init

cd "$(dirname $0)/../../.."

cmake . -B${BUILD_DIR} -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
cmake --build "${BUILD_DIR}" --target kagome -- -j${BUILD_THREADS}
