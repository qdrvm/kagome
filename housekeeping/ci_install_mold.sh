#!/bin/bash -xe
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

if [ "$CI" != "true" ] || [ "$GITHUB_ACTIONS" != "true" ]; then
  echo "CI=true GITHUB_ACTIONS=true are required"
  exit -1
fi

wget -q -O- https://github.com/rui314/mold/releases/download/v1.3.1/mold-1.3.1-x86_64-linux.tar.gz | tar -C /usr/local --strip-components=1 -xzf -
if [ "$1" == "--make-default" ]; then
  ln -sf /usr/local/bin/mold $(realpath /usr/bin/ld)
fi
