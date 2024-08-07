#!/bin/bash -xe
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

BUILD_DIR="${BUILD_DIR:-build}"

cd $(dirname $0)/..
# exclude WAVM because on CI clang-tidy is run on a WasmEdge build
git diff -U0 origin/master -- . ':!core/runtime/wavm' | clang-tidy-diff.py -p1 -path $BUILD_DIR -iregex '(core|node)\/.*\.(h|c|hpp|cpp)' -clang-tidy-binary clang-tidy-16 | tee clang-tidy.log
! grep ': error:' clang-tidy.log
