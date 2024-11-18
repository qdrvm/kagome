#!/bin/bash -ex
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

cd "$(dirname "$0")"
echo "Building in $(pwd)"

VERSION=${VERSION:-5}
TAG=qdrvm/kagome-dev:$VERSION

docker build -t ${TAG}-runner -f kagome_runner.Dockerfile .
docker push ${TAG}-runner
