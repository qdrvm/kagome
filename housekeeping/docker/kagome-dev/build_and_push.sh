#!/bin/bash -ex

cd "$(dirname "$0")"

VERSION=3.1
TAG=harrm/kagome-dev:$VERSION

docker build -t ${TAG}-minideb -f minideb.Dockerfile .
docker push ${TAG}-minideb
