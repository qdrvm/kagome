#!/bin/bash -ex
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

if [[ "${CI}" ]]; then
  git config --global --add safe.directory /__w/kagome/kagome
fi

if [[ "${KAGOME_IN_DOCKER}" = 1 ]]; then
  source /venv/bin/activate
fi

KAGOME_ROOT="$(dirname "$0")/../../.."
echo "KAGOME_ROOT: $KAGOME_ROOT"

# cd to kagome source root
cd "$KAGOME_ROOT"

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
echo "BUILD_DIR: $BUILD_DIR"

BUILD_TYPE="${BUILD_TYPE:?BUILD_TYPE variable is not defined}"

if [ "$BUILD_TYPE" != "Debug" ] && [ "$BUILD_TYPE" != "Release" ] && [ "$BUILD_TYPE" != "RelWithDebInfo" ] && [ "$BUILD_TYPE" != "Custom" ]; then
  echo "Invalid build type $BUILD_TYPE, should be either Debug, Release or RelWithDebInfo"
  exit 1
fi

VERSION="${VERSION:?VERSION variable is not defined}"

# For github action we need remove ref prefix
if [ "$VERSION" = "refs/heads/master" ]; then
  VERSION=latest
elif [[ "$VERSION"  == refs/tags/* ]]; then
  VERSION="${VERSION#refs/tags/}"
else
  VERSION=devops
fi

if [ "$BUILD_TYPE" = "Debug" ]; then
  VERSION="${VERSION}-debug"
fi
if [ "$BUILD_TYPE" = "RelWithDebInfo" ]; then
  VERSION="${VERSION}-rel-with-deb-info"
fi

if [ "$BUILD_TYPE" = "Custom" ]; then
  DOCKER_USERNAME="$(docker info | sed '/Username:/!d;s/.* //')"
  COMMIT_HASH="$(git rev-parse --short HEAD)"
  TAG="$DOCKER_USERNAME/kagome:$COMMIT_HASH"
else
  TAG="qdrvm/kagome:$VERSION"
fi

CTX_DIR="${BUILD_DIR}/docker_context"
echo "CTX_DIR: $CTX_DIR"

# Cleanup docker context
rm -Rf ${CTX_DIR}
mkdir -p ${CTX_DIR}

# Copy binaries
cp -a ${BUILD_DIR}/node/kagome ${CTX_DIR}/

# wasmedge shared library
cp $(ldd cmake-build-debug-wasmedge-clang-16/node/kagome | grep -o /.*/lib/libwasmedge.so[.0-9]*) ${CTX_DIR}/

if [ "$BUILD_TYPE" = "Release" ] || [ "$BUILD_TYPE" = "Custom" ]; then
  docker build -t $TAG -f housekeeping/docker/kagome/minideb-release.Dockerfile ${CTX_DIR}

elif [ "$BUILD_TYPE" = "Debug" ]; then
  docker build -t $TAG -f housekeeping/docker/kagome/minideb-debug.Dockerfile ${CTX_DIR}


elif [ "$BUILD_TYPE" = "RelWithDebInfo" ]; then
  docker build -t $TAG -f housekeeping/docker/kagome/minideb-release.Dockerfile ${CTX_DIR}

else
  echo "Unknown build type: $BUILD_TYPE"
  exit 1
fi

docker push $TAG

# Push with commit hash if not custom
if [ "$BUILD_TYPE" != "Custom" ]; then

HASH_COMMIT="$(git rev-parse --short HEAD)"
TAG_HASH_COMMIT="qdrvm/kagome:$HASH_COMMIT"

if [ "$BUILD_TYPE" = "Debug" ]; then
  TAG_HASH_COMMIT="${TAG_HASH_COMMIT}-debug"
fi
if [ "$BUILD_TYPE" = "RelWithDebInfo" ]; then
  TAG_HASH_COMMIT="${TAG_HASH_COMMIT}-rel-with-deb-info"
fi

echo "$TAG_HASH_COMMIT" > tag_output.txt

docker tag $TAG $TAG_HASH_COMMIT
docker push $TAG_HASH_COMMIT

fi

rm -R ${CTX_DIR}
