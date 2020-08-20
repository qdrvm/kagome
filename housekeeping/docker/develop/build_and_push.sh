#!/bin/bash -ex

cd $(dirname $0)

VERSION=10
TAG=soramitsu/kagome-dev:$VERSION

docker build -t $TAG .
docker push $TAG

docker build -t ${TAG}-alpine -f Dockerfile-alpine .
docker push ${TAG}-alpine
