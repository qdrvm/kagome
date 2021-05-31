![logo](/docs/image_assets/kagome-logo-(new-2020).svg)

[![code inspector](https://www.code-inspector.com/project/74/status/svg)](https://www.code-inspector.com/public/project/74/kagome/dashboard)
[![CodeFactor](https://www.codefactor.io/repository/github/soramitsu/kagome/badge)](https://www.codefactor.io/repository/github/soramitsu/kagome)
[![codecov](https://codecov.io/gh/soramitsu/kagome/branch/master/graph/badge.svg)](https://codecov.io/gh/soramitsu/kagome)
[![Netlify Status](https://api.netlify.com/api/v1/badges/ad6fa504-99d6-48fb-9a05-869ba1d9a7c3/deploy-status)](https://app.netlify.com/sites/kagome/deploys)
[![](https://img.shields.io/twitter/follow/Soramitsu_co?label=Follow&style=social)](https://twitter.com/Soramitsu_co)

## Intro

Kagome is a [Polkadot Host](https://github.com/w3f/polkadot-spec/tree/master/host-spec) (former Polkadot Runtime Environment) developed by [Soramitsu](https://soramitsu.co.jp/) and funded by Web3 Foundation [grant](https://github.com/w3f/Web3-collaboration/blob/master/grants/grants.md). 


## Status

Kagome is early-stage software, you can already execute a block production process.

A simple status-report can be found within section [supported features](./README.md/#supported-features).



## Getting Started

### Prerequisites

For now, please refer to the [Dockerfile](housekeeping/docker/kagome-dev/minideb.Dockerfile) to get a picture of what you need for a local build-environment.

### Build

```sh
git clone https://github.com/soramitsu/kagome
cd kagome

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make kagome -j 

```
## Build with docker

```sh
git clone https://github.com/soramitsu/kagome
cd kagome

# build and run tests
INDOCKER_IMAGE=soramitsu/kagome-dev:11 BUILD_DIR=build BUILD_THREADS=9 ./housekeeping/indocker.sh ./housekeeping/makeBuild.sh

# You can use indocker.sh to run any script or command inside docker
# It mounts project dir and copy important env variable inside the container.
INDOCKER_IMAGE=soramitsu/kagome-dev:11 ./housekeeping/indocker.sh gcc --version

## Build Release 
# Build Kagome
INDOCKER_IMAGE=soramitsu/kagome-dev:11 BUILD_DIR=build ./housekeeping/indocker.sh ./housekeeping/docker/release/makeRelease.sh
# Create docker image and push 
VERSION=0.0.1 BUILD_DIR=build ./housekeeping/docker/release/build_and_push.sh
# or just build docker image 
docker build -t soramitsu/kagome:0.0.1 -f ./housekeeping/docker/release/Dockerfile ./build

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

---
**Note**

At the moment launch from the existing db is not implemented, so you should clean up previous db before every launch using the following command from the chain folder in the base path:
```
rm -rf base_path/dev/db
```
---

To launch Kagome validator execute:
```
cd examples/first_kagome_chain
PATH=$PATH:../../build/node/
kagome --validator --chain localchain.json --base-path base_path
```

This command executes Kagome full node with authority role which can receive extrinsics locally on port using http: `9933`. Simple transfer transaction can be sent as follows:
```
curl -H "Content-Type: application/json" -d '{"id":1, "jsonrpc":"2.0", "method": "author_submitExtrinsic", "params": ["0x290284ffdc3488acc1a6b90aa92cea0cfbe2b00754a74084970b08d968e948d4d3bf161a01e2f2be0a634faeb8401ed2392731df803877dcb2422bb396d48ca24f18661059e3dde41d14b87eb929ec41ab36e6d63be5a1f5c3c5c092c79646a453f4b392890000000600ff488f6d1b0114674dcd81fd29642bc3bcec8c8366f6af0665860f9d4e8c8a972404"]}' http://localhost:9933/
```
If transaction was successfully applied we should see the following output:
```
{"jsonrpc":"2.0","id":1,"result":[194,108,28,60,223,55,48,163,134,182,201,23,144,126,167,123,33,119,187,164,61,50,203,175,230,189,71,245,120,104,18,38]}% 
```


### Execute Kagome Polkadot archive node

You can synchronize with Polkadot using Kagome and obtain archive node that can be used to query Polkadot network at any state. 

---
**Note**

Same note as for validating node. At the moment launch from existing db is not implemented, so you should clean up previous db before every launch using the following command from the chain folder in the base path:
```
rm -rf syncing_chain
```
---

To launch Kagome Polkadot archive node execute:
```
cd examples/polkadot/
PATH=$PATH:../../build/node/
kagome --chain polkadot.json --base-path syncing_chain --port 50541 --rpc-port 50542 --ws-port 50543 --unix-slots
```

After this command syncing node will connect with the full node and start importing blocks.


---
**Note**
The ports, which are not set in the app arguments, will take a default value. In case of running two nodes on the same address, this may lead to address collision and one node will node be able to start. To avoid this, please set all ports when running several nodes on one machine.
___

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
    * Uses [Binaryen](https://github.com/WebAssembly/binaryen) WASM interpreter to invoke Polkadot Runtime entries
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
    * Kagome uses [cpp-libp2p](https://github.com/soramitsu/libp2p) for peer-to-peer interactions and peer discovery
    * Gossiper and Gossiper observer
    * SyncClient and SyncServer 
    
You can find more information about the components by checking [reference documentation](https://kagome.netlify.com). 

## Kagome in media

* Press-release: [Soramitsu to implement Polkadot Runtime Environment in C++](https://medium.com/web3foundation/w3f-grants-soramitsu-to-implement-polkadot-runtime-environment-in-c-cf3baa08cbe6)
* [Kagome: C++ implementation of PRE](https://www.youtube.com/watch?v=181mk2xvBZ4&t=) presentation at DOTCon (18.08.19)
* [Kagome and consensus in Polkadot](https://www.youtube.com/watch?v=5OrevTjaiPA) presentation (in Russian) during Innopolis blockchain meetup (28.10.19)
* [Web3 Builders: Soramitsu | C++ Implementation of Polkadot Host](https://www.youtube.com/watch?v=We3kiGzg60w) Polkadot's Web3 builders online presentation 
