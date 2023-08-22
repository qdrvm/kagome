#!/bin/bash -xe

BUILD_DIR=build

BUILD_TYPE="${BUILD_TYPE:?BUILD_TYPE variable is not defined}"

if [ "$BUILD_TYPE" != "Debug" ] && [ "$BUILD_TYPE" != "Release" ] && [ "$BUILD_TYPE" != "RelWithDebInfo" ]; then
  echo "Invalid build type $BUILD_TYPE, should be either Debug, Release or RelWithDebInfo"
fi

if [ "$BUILD_TYPE" = "Release" ]; then
  BUILD_THREADS=1
elif [ "$BUILD_TYPE" = "RelWithDebInfo" ]; then
  BUILD_THREADS=2
else
  BUILD_THREADS="${BUILD_THREADS:-$(( $(nproc 2>/dev/null || sysctl -n hw.ncpu) / 2 + 1 ))}"
fi

git submodule update --init

cd "$(dirname $0)/../../.."

cmake . -B"${BUILD_DIR}" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
cmake --build "${BUILD_DIR}" --target kagome -- -j${BUILD_THREADS}
