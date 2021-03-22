#!/bin/bash -ex

cd "$(dirname "$0")"

VERSION=1
TAG=harrm/kagome-dev:$VERSION

docker build -t ${TAG}-ubuntu-build -f ubuntu-build.Dockerfile .
docker push ${TAG}-ubuntu-build

docker build -t ${TAG}-minideb-build -f minideb-build.Dockerfile .
docker push ${TAG}-minideb-build
