#!/bin/bash -xe

BUILD_DIR="${BUILD_DIR:-build}"

cd $(dirname $0)/..
git diff -U0 HEAD^ | clang-tidy-diff.py -p1 -path $BUILD_DIR -regex "\.(hpp|h)"
