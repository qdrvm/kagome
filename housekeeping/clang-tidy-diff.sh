#!/bin/bash -xe
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

BUILD_DIR="${BUILD_DIR:-build}"
CLANG_TIDY_BIN="${CLANG_TIDY_BIN:-clang-tidy-16}"

cd $(dirname $0)/..
# exclude WAVM because on CI clang-tidy is run on a WasmEdge build
git diff -U0 origin/master -- . ':!core/runtime/wavm' | clang-tidy-diff.py -p1 -path ${BUILD_DIR} -iregex '(core|node)\/.*\.(h|c|hpp|cpp)' -clang-tidy-binary ${CLANG_TIDY_BIN} | tee clang-tidy.log
! grep ': error:' clang-tidy.log
