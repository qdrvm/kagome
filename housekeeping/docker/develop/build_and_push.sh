#!/bin/bash -ex

cd $(dirname $0)

VERSION=1
TAG=soramitsu/kagome-dev:$VERSION

docker build -t $TAG .
docker push $TAG

docker build -t ${TAG}-minideb -f Dockerfile-minideb .
docker push ${TAG}-minideb
