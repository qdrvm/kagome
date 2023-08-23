#!/bin/bash -xe

BUILD_DIR="${BUILD_DIR:?BUILD_DIR variable is not defined}"
BUILD_FINAL_TARGET="${BUILD_FINAL_TARGET:-test}"
BUILD_THREADS="${BUILD_THREADS:-$(( $(nproc 2>/dev/null || sysctl -n hw.ncpu) / 2))}"

which git

cd "$(dirname $0)/.."

if [ -d "/__w/kagome/kagome" ]
then
  echo "Directory /__w/kagome/kagome exists. Updating safe.directory"
  git config --global --add safe.directory /__w/kagome/kagome
  source /venv/bin/activate
fi

cmake . -B${BUILD_DIR} "$@"
if [ "$BUILD_FINAL_TARGET" != "generated" ] ; then
  cmake --build "${BUILD_DIR}" -- -j${BUILD_THREADS}
fi
cmake --build "${BUILD_DIR}" --target "${BUILD_FINAL_TARGET}"
