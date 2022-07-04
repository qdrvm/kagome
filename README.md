![logo](/docs/image_assets/kagome-logo-(new-2020).svg)

[![CodeFactor](https://www.codefactor.io/repository/github/soramitsu/kagome/badge)](https://www.codefactor.io/repository/github/soramitsu/kagome)
[![codecov](https://codecov.io/gh/soramitsu/kagome/branch/master/graph/badge.svg)](https://codecov.io/gh/soramitsu/kagome)
[![Netlify Status](https://api.netlify.com/api/v1/badges/ad6fa504-99d6-48fb-9a05-869ba1d9a7c3/deploy-status)](https://app.netlify.com/sites/kagome/deploys)
[![](https://img.shields.io/twitter/follow/Soramitsu_co?label=Follow&style=social)](https://twitter.com/Soramitsu_co)

## Intro

Kagome is a [Polkadot Host](https://github.com/w3f/polkadot-spec/tree/master/host-spec) (former Polkadot Runtime Environment) developed by [Soramitsu](https://soramitsu.co.jp/) and funded by a Web3 Foundation [grant](https://github.com/w3f/Web3-collaboration/blob/master/grants/grants.md).


## Status

Kagome is getting closer to the first official release. Team is currently focused on stabilizing features required to run syncing and validating full nodes.

A simple status of Kagome development can be found within the [supported features](./README.md/#supported-features) section.



## Getting Started

### Prerequisites

For now, please refer to the [Dockerfile](housekeeping/docker/kagome-dev/minideb.Dockerfile) to get a picture of what you need for a local build-environment.

### Build

```sh
git clone https://github.com/soramitsu/kagome
cd kagome

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make kagome -j 4
```

`make kagome -j <numbers of cores>` (should be less than you have)


### Build with docker

```sh
git clone https://github.com/soramitsu/kagome
cd kagome

# build and run tests
INDOCKER_IMAGE=soramitsu/kagome-dev:4-minideb BUILD_DIR=build BUILD_THREADS=9 ./housekeeping/indocker.sh ./housekeeping/make_build.sh

# You can use indocker.sh to run any script or command inside docker
# It mounts project dir and copy important env variable inside the container.
INDOCKER_IMAGE=soramitsu/kagome-dev:4-minideb ./housekeeping/indocker.sh gcc --version

## Build Release 
# Build Kagome
INDOCKER_IMAGE=soramitsu/kagome-dev:4-minideb BUILD_DIR=build ./housekeeping/indocker.sh ./housekeeping/docker/release/makeRelease.sh
# Create docker image and push 
VERSION=0.0.1 BUILD_DIR=build BUILD_TYPE=Release ./housekeeping/docker/kagome/build_and_push.sh

# Check docker image 
docker run -it --rm soramitsu/kagome:0.0.1 kagome
[2020-06-03 16:26:14][error] the option '--chain' is required but missing

```

### Execute kagome node in development mode

The easiest way to get started with Kagome is to execute in development mode which is a single node network:

```
kagome --dev
```

That executes node with default accounts Alice and Bob. You can read about these accounts [here](https://kagome.readthedocs.io/en/latest/tutorials/first_kagome_chain.html#launch-kagome-network).

To launch with wiping existing data you can do:

```
kagome --dev-with-wipe
```

### Execute Kagome node with validator mode

To launch Kagome validator execute:
```
cd examples/first_kagome_chain
PATH=$PATH:../../build/node/
kagome --validator --chain localchain.json --base-path base_path
```

This command executes Kagome full node with authority role.


### Execute Kagome Polkadot archive node

You can synchronize with Polkadot using Kagome and obtain archive node that can be used to query Polkadot network at any state.

To launch Kagome Polkadot archive node execute:
```
cd examples/polkadot/
PATH=$PATH:../../build/node/
kagome --chain polkadot.json --base-path syncing_chain
```

After this command syncing node will connect with the full node and start importing blocks.



### Configuration Details
To run a Kagome node, you need to provide to it a genesis config, cryptographic keys and a place to store db files.
* Example of a genesis config file can be found in `examples/first_kagome_chain/localchain.json`
* Example of a base path dir can be found in `examples/first_kagome_chain/base_path`
* To create leveldb files, just provide any base path into `kagome` executable (mind that start with authority role requires keys to start).


### Build Kagome

First build will likely take long time. However, you can cache binaries to [hunter-binary-cache](https://github.com/soramitsu/hunter-binary-cache) or even download binaries from the cache in case someone has already compiled project with the same compiler. To this end, you need to set up two environment variables:
```
GITHUB_HUNTER_USERNAME=<github account name>
GITHUB_HUNTER_TOKEN=<github token>
```
To generate github token follow the [instructions](https://help.github.com/en/github/authenticating-to-github/creating-a-personal-access-token-for-the-command-line). Make sure `read:packages` and `write:packages` permissions are granted (step 7 in instructions).

This project is can be built with

```
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
```

Tests can be run with:
```
cd build
ctest
```

## Contributing Guides

Please refer to the [Contributor Documentation](./docs/source/development/dev-guide.md).

## Supported Features
* Extrinsic Api
    * Receives extrinsics submitted over JSON-RPC, validates them and stores in transaction pool
* Transaction pool
    * Stores submitted transactions
    * Provides ready transactions, i.e. transaction which pre-conditions defined by tags are satisfied
* Authorship
    * Creates a block from provided inherents and digest
    * Interacts with Polkadot Runtime to initialize and finalize block
* Runtime
    * Uses [Binaryen](https://github.com/WebAssembly/binaryen) (default) or [WAVM](https://wavm.github.io/) (experimental) WASM executors to invoke Polkadot Runtime entries
    * Current runtime entries include:
        * BlockBuilder – checks inherents, applies extrinsics, derives inherent extrinsics, initializes and finalizes blocks
        * Core – gets version of runtime, executes blocks, gets authorities
        * Grandpa – gets grandpa authorities
        * Metadata
        * OffchainWorker (not used)
        * ParachainHost (not used)
        * TaggedTransactionQueue – validates transactions
* Host Api (aka Extensions)
    * Exposes a set functions that Runtime needs for:
        * Management of the content
        * Memory allocation
        * Cryptography
* [SCALE](https://github.com/soramitsu/scale-codec-cpp) (polkadot-codec)
    * Implements SCALE codec for data types serialization according to [spec](https://substrate.dev/docs/en/conceptual/core/codec).
* Storage
    * Contains of key-value storage interfaces with LevelDB- and inmemory-based implementations
    * Merkle-Patricia trie implementation, described in spec
* Clock
    * Gets current time for the peer
* Consensus
    * Babe block production
    * Grandpa finalization
* Crypto
    * blake2
    * ed25519
    * keccak
    * sha
    * Schnorr's vrf and sr25519 (bindings over Web3’s [schnorrkel library](https://github.com/w3f/schnorrkel))
    * twox
* Networking
    * Kagome uses [cpp-libp2p](https://github.com/soramitsu/libp2p) for peer-to-peer interactions and peer discovery
    * Gossiper and Gossiper observer
    * SyncClient and SyncServer

You can find more information about the components by checking [reference documentation](https://kagome.netlify.com). Check out tutorials and more examples in official documentation: https://kagome.readthedocs.io/

## Kagome in media

* Press-release: [Soramitsu to implement Polkadot Runtime Environment in C++](https://medium.com/web3foundation/w3f-grants-soramitsu-to-implement-polkadot-runtime-environment-in-c-cf3baa08cbe6)
* [Kagome: C++ implementation of PRE](https://www.youtube.com/watch?v=181mk2xvBZ4&t=) presentation at DOTCon (18.08.19)
* [Kagome and consensus in Polkadot](https://www.youtube.com/watch?v=5OrevTjaiPA) presentation (in Russian) during Innopolis blockchain meetup (28.10.19)
* [Web3 Builders: Soramitsu | C++ Implementation of Polkadot Host](https://www.youtube.com/watch?v=We3kiGzg60w) Polkadot's Web3 builders online presentation 
