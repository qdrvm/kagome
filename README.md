![logo](/docs/image_assets/logo.png)

[![Build Status](https://travis-ci.org/soramitsu/kagome.svg?branch=master)](https://travis-ci.org/soramitsu/kagome)
[![code inspector](https://www.code-inspector.com/project/74/status/svg)](https://www.code-inspector.com/public/project/74/kagome/dashboard)
[![CodeFactor](https://www.codefactor.io/repository/github/soramitsu/kagome/badge)](https://www.codefactor.io/repository/github/soramitsu/kagome)
[![codecov](https://codecov.io/gh/soramitsu/kagome/branch/master/graph/badge.svg)](https://codecov.io/gh/soramitsu/kagome)
[![Netlify Status](https://api.netlify.com/api/v1/badges/ad6fa504-99d6-48fb-9a05-869ba1d9a7c3/deploy-status)](https://app.netlify.com/sites/kagome/deploys)
[![](https://img.shields.io/twitter/follow/Soramitsu_co?label=Follow&style=social)](https://twitter.com/Soramitsu_co)

## Intro

Kagome is a [Polkadot Runtime Environment](https://github.com/w3f/polkadot-spec/tree/master/runtime-environment-spec) developed by [Soramitsu](https://soramitsu.co.jp/) add funded by Web3 Foundation [grant](https://github.com/w3f/Web3-collaboration/blob/master/grants/grants.md). 


## Status

Kagome is early-stage software, you can already execute a block production process.

A simple status-report can be found within section [supported features](./README.md/#supported-features).



## Getting Started

### Prerequisites

For now, please refer to the [Dockerfile](./housekeeping/docker/Dockerfile) to get a picture of what you need for a local build-environment.

### Build

```sh
git clone --recurse-submodules https://github.com/soramitsu/kagome
cd kagome

# Only needed if you forgot `--recurse-submodules` above
git submodule update --init --recursive

mkdir build && cd build
cmake ..
make kagome_full -j 
```



### Execute kagome node
To launch kagome node execute:
```
cd examples/kagome_full
kagome_full --genesis config/polkadot-v06.json --keystore config/keystore.json -l ldb
```

This command executes kagome node which can receive extrinsics locally on port: `4224` (currently hardcoded) 

### Configuration Details
* Executable binary could be found in `build/examples/kagome_full/`
* To execute kagome node you need to provide it with genesis config, keys and leveldb files
* Example genesis config file can be found in `examples/kagome_full/config/polkadot-v06.json`
* Example keys file can be found in `examples/kagome_full/config/keystore.json`
* To create leveldb storage file just provide any path into `kagome_full` executable.


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
cmake -DCLANG_TIDY=ON ..
make -j
```

It is suggested to build project with clang-tidy checks, however if you wish to omit clang-tidy step, you can use `cmake ..` instead.

Tests can be run with: 
```
cd build
ctest
```

## Contributing Guides

Please refer to the [Contributor Documentation](./docs/README.md).


## Kagome in media

* Press-release: [Soramitsu to implement Polkadot Runtime Environment in C++](https://medium.com/web3foundation/w3f-grants-soramitsu-to-implement-polkadot-runtime-environment-in-c-cf3baa08cbe6)
* [Kagome: C++ implementation of PRE](https://www.youtube.com/watch?v=181mk2xvBZ4&t=) presentation at DOTCon (18.08.19)
* [Kagome and consensus in Polkadot](https://www.youtube.com/watch?v=5OrevTjaiPA) presentation (in Russian) during Innopolis blockchain meetup (28.10.19)

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
    * Uses [Binaryen](https://github.com/WebAssembly/binaryen) WASM interpreter to invoke Polkadot Runtime entries
    * Current runtime entries include:
        * BlockBuilder – checks inherents, applies extrinsics, derives inherent extrinsics, initializes and finalizes blocks
        * Core – gets version of runtime, executes blocks, gets authorities
        * Grandpa – gets grandpa authorities
        * Metadata (not used)
        * OffchainWorker (not used)
        * ParachainHost (not used)
        * TaggedTransactionQueue – validates transactions
* Externals (aka Extensions)
    * Exposes a set functions that Runtime needs for:
        * Management of the content
        * Memory allocation
        * Cryptography
* SCALE (polkadot-codec)
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
    * Kagome uses [cpp-libp2p](https://github.com/soramitsu/libp2p) for peer-to-peer interactions
    * Gossiper and Gossiper observer
    * SyncClient and SyncServer 
    
You can find more information about the components by checking [reference documentation](https://kagome.netlify.com). 

