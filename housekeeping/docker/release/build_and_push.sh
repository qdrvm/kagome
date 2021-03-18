#!/bin/bash -ex

cd "$(dirname $0)/../../.."

BUILD_DIR=${BUILD_DIR:-`pwd`/build}

VERSION="${VERSION:?VERSION variable is not defined}"
# For github action we need remove ref prefix
if [ "$VERSION" = "refs/heads/master" ]; then
  VERSION=latest
elif [[ "$VERSION"  == refs/tags/* ]]; then
  VERSION="${VERSION#refs/tags/}"
else
  VERSION=$VERSION
fi

TAG=harrm/kagome:$VERSION

CTX_DIR=${BUILD_DIR}/docker_context

# Cleanup docker context
rm -Rf ${CTX_DIR}
mkdir -p ${CTX_DIR}

# Copy binaries
cp -a ${BUILD_DIR}/node/kagome_full_syncing/kagome_full_syncing ${CTX_DIR}/
cp -a ${BUILD_DIR}/node/kagome_validating/kagome_validating  ${CTX_DIR}/

# Strip binaries
# strip ${CTX_DIR}/kagome_full_syncing
# strip ${CTX_DIR}/kagome_validating

# Make build
docker build -t $TAG -f housekeeping/docker/release/Dockerfile-minideb ${CTX_DIR}
docker push $TAG

rm -R ${CTX_DIR}
