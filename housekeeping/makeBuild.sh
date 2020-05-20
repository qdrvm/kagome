#!/bin/bash -xe

BUILD_DIR="${BUILD_DIR:?BUILD_DIR variable is not defined}"
BUILD_TARGET="${BUILD_TARGET:-test}"

which git

cd "$(dirname $0)/.."

git pull --recurse-submodules
git submodule update --init --recursive

cmake . -B${BUILD_DIR} "$@"
cmake --build "${BUILD_DIR}" -- -j4
cmake --build "${BUILD_DIR}" --target "${BUILD_TARGET}"
