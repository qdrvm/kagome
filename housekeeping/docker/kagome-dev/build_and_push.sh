#!/bin/bash -ex

cd "$(dirname "$0")"

VERSION=3
TAG=soramitsu/kagome-dev:$VERSION

docker build -t ${TAG}-minideb -f minideb.Dockerfile .
docker push ${TAG}-minideb
