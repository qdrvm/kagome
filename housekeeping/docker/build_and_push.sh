#!/bin/bash -ex

cd $(dirname $0)

VERSION=6
TAG=soramitsu/kagome-dev:$VERSION

docker build -t $TAG .
docker push $TAG
