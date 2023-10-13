#!/bin/bash -ex
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

cd "$(dirname "$0")"

VERSION=${VERSION:-4}
TAG=qdrvm/kagome-dev:$VERSION

docker build -t ${TAG}-minideb -f minideb.Dockerfile .
docker push ${TAG}-minideb
