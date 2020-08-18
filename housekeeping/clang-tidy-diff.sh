#!/bin/bash -xe

# TODO delete this script after travis removing

cd $(dirname $0)/..
git diff -U0 HEAD^ | clang-tidy-diff.py -p1
