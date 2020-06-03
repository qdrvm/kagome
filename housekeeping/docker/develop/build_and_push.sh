#!/bin/bash -ex

cd $(dirname $0)

VERSION=9
TAG=soramitsu/kagome-dev:$VERSION

docker build -t $TAG .
docker push $TAG
