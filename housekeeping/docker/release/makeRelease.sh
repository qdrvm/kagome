#!/bin/bash -xe

BUILD_DIR=build
BUILD_TREADS="${BUILD_TREADS:-4}"

cd "$(dirname $0)/../../.."

cmake . -B${BUILD_DIR} -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --target kagome_full_syncing kagome_validating kagome_block_producing -- -j${BUILD_TREADS}
