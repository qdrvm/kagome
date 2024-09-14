#!/bin/bash -xe
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

BUILD_DIR="${BUILD_DIR:-build}"
CLANG_TIDY_BIN="${CLANG_TIDY_BIN:-clang-tidy}"
CI="${CI:-false}"

if [ "${CI}" == "true" ]; then
  git fetch origin master
fi

cd $(dirname $0)/..
HUNTER_INCLUDE=$(cat $BUILD_DIR/_3rdParty/Hunter/install-root-dir)/include
# exclude WAVM because on CI clang-tidy is run on a WasmEdge build
git diff -U0 origin/master -- . ':!core/runtime/wavm' | \
clang-tidy-diff.py \
  -p1 \
  -path ${BUILD_DIR} \
  -iregex '(core|node)\/.*\.(h|c|hpp|cpp)' \
  -clang-tidy-binary ${CLANG_TIDY_BIN} \
  -- \
    --std=c++20 \
    -I core \
    -I $BUILD_DIR \
    -I $BUILD_DIR/pb/authority_discovery_proto/generated \
    -I $BUILD_DIR/pb/light_api_proto/generated \
    -I $BUILD_DIR/pb/node_api_proto/generated \
    -I $HUNTER_INCLUDE \
    -I $HUNTER_INCLUDE/binaryen \
| tee clang-tidy.log
! grep ': error:' clang-tidy.log
