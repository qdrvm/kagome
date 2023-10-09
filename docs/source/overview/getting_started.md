[//]: # (
Copyright Quadrivium LLC
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Overview

## Getting started

### Prerequisites

For now, please refer to the [Dockerfile](https://github.com/soramitsu/kagome/blob/master/housekeeping/docker/kagome-dev/minideb.Dockerfile) to get a picture of what you need for a local build-environment.

### Clone

```sh
git clone --recurse-submodules https://github.com/soramitsu/kagome
cd kagome

# Only needed if you did not use `--recurse-submodules` above
git submodule update --init --recursive

```

### Build

First build will likely take long time. However, you can cache binaries to [hunter-binary-cache](https://github.com/soramitsu/hunter-binary-cache) or even download binaries from the cache in case someone has already compiled project with the same compiler. To this end, you need to set up two environment variables:
```
GITHUB_HUNTER_USERNAME=<github account name>
GITHUB_HUNTER_TOKEN=<github token>
```
To generate github token follow the [instructions](https://help.github.com/en/github/authenticating-to-github/creating-a-personal-access-token-for-the-command-line). Make sure `read:packages` and `write:packages` permissions are granted (step 7 in instructions).

#### Build all

This project is can be built with

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j 
```

Tests can be run with:
 
```
cd build
ctest
```

#### Build node application

If you'd like to build node use the following instruction

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make kagome -j
```
