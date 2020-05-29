#!/bin/bash -ex

cd "$(dirname $0)/../../.."

BUILD_DIR=build
VERSION="${VERSION:?VERSION variable is not defined}"
# For github action we need remove ref prefix
if [ "$VERSION" = "refs/heads/master" ]; then
  VERSION=latest
elif [[ "$VERSION"  == refs/tags/* ]]; then
  VERSION="${VERSION#refs/tags/}"
else
  VERSION=$VERSION
fi

TAG=soramitsu/kagome:$VERSION

docker build -t $TAG -f housekeeping/docker/release/Dockerfile ${BUILD_DIR}
docker push $TAG