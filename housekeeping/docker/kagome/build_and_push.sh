#!/bin/bash -ex

KAGOME_ROOT="$(dirname "$0")/../../.."

# cd to kagome source root
cd "$KAGOME_ROOT"

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"

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
  VERSION=$VERSION
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
  TAG="soramitsu/kagome:$VERSION"
fi

CTX_DIR="${BUILD_DIR}/docker_context"

# Cleanup docker context
rm -Rf ${CTX_DIR}
mkdir -p ${CTX_DIR}

# Copy binaries
cp -a ${BUILD_DIR}/node/kagome ${CTX_DIR}/

if [ "$BUILD_TYPE" = "Release" ] || [ "$BUILD_TYPE" = "Custom" ]; then
  strip ${CTX_DIR}/kagome

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

rm -R ${CTX_DIR}
