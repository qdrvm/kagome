![logo](/docs/image_assets/kagome-logo-(new-2020).svg)

[![CodeFactor](https://www.codefactor.io/repository/github/soramitsu/kagome/badge)](https://www.codefactor.io/repository/github/soramitsu/kagome)
[![codecov](https://codecov.io/gh/soramitsu/kagome/branch/master/graph/badge.svg)](https://codecov.io/gh/soramitsu/kagome)
[![Netlify Status](https://api.netlify.com/api/v1/badges/ad6fa504-99d6-48fb-9a05-869ba1d9a7c3/deploy-status)](https://app.netlify.com/sites/kagome/deploys)
[![](https://img.shields.io/twitter/follow/Soramitsu_co?label=Follow&style=social)](https://twitter.com/Soramitsu_co)

## Intro

KAGOME is a [Polkadot Host](https://github.com/w3f/polkadot-spec/tree/master/host-spec) (former Polkadot Runtime Environment) developed by [Soramitsu](https://soramitsu.co.jp/) and funded by a Web3 Foundation [grant](https://github.com/w3f/Web3-collaboration/blob/master/grants/grants.md) and Kusama [treasury](https://kusama.polkassembly.io/post/1858).


## Status

- [x] Syncing node
    - Polkadot, Kusama and Rococo compatibility
- [x] Validating node
- [x] Polkadot JS apps support
- [ ] Parachains support
   - [x] Non-asynchronous Backing
   - [x] Data availability
   - [x] Approval voting
   - [ ] Disputes resolution
- [x] Offchain workers
- [x] Telemetry support
- [x] Prometheus metrics
- [x] Fast sync
- [ ] Warp sync
   - [x] Incoming connections
   - [ ] Outcoming connections
- [x] Light client protocol

More details of KAGOME development can be found within the [supported features](./README.md/#supported-features) section and in [projects board](https://github.com/soramitsu/kagome/projects/2)

## Getting Started

### Build

#### Prerequisites

For now, please refer to the [Dockerfile](housekeeping/docker/kagome-dev/minideb.Dockerfile) to get a picture of what you need for a local build-environment.


```sh
git clone https://github.com/soramitsu/kagome
cd kagome

make build
```

You will get KAGOME binary in the `build/node/` folder

Other make commands are:

```
make docker
make command args="gcc --version"
make release
make release_docker
make debug_docker
make clear
```

### Executing KAGOME node

#### Obtaining database snapshot (optional)

In order to avoid syncing from scratch we are maintaining the most recent snapshot of Polkadot network for KAGOME node available for anyone here: https://drive.google.com/drive/folders/1pAZ1ongWB3_zVPKXvgOo-4aBB7ybmKy5?usp=sharing

After downloading the snapshot you can extract it in the folder where the node will be running:

```
unzip polkadot-node-1.zip
```

#### Execute KAGOME Polkadot full syncing node

You can synchronize with Polkadot using KAGOME and obtain an archive node that can be used to query the Polkadot network at any state.

To launch KAGOME Polkadot syncing node execute:
```
cd examples/polkadot/
PATH=$PATH:../../build/node/
kagome --chain polkadot.json --base-path polkadot-node-1
```

> Note: If you start KAGOME for the first time, you can add the `--sync Fast` flag to synchronize using [Fast sync](https://spec.polkadot.network/#sect-sync-fast)

After this command KAGOME will connect with other nodes in the network and start importing blocks. You may play with your local node using polkadot js apps: https://polkadot.js.org/apps/?rpc=ws%3A%2F%2F127.0.0.1%3A9944#/explorer

You will also be able to see your node on https://telemetry.polkadot.io/. If you need to identify it more easily you can add `--name <node-name>` flag to node's execution command and find your node in telemetry by typing its name.

Run `kagome --help` to explore other CLI flags.


#### Execute KAGOME validating node in development mode

The easiest way to get started with KAGOME is to run it in development mode, which is a single node network:

```
kagome --dev
```

That executes node with default accounts Alice and Bob. You can read about these accounts [here](https://kagome.readthedocs.io/en/latest/tutorials/first_kagome_chain.html#launch-kagome-network).

To launch with wiping existing data you can do:

```
kagome --dev-with-wipe
```

#### Run KAGOME node in validator mode

To start the KAGOME validator:
```
cd examples/first_kagome_chain
PATH=$PATH:../../build/node/
kagome --validator --chain localchain.json --base-path base_path
```

This command executes a KAGOME full node with an authority role.


### Configuration Details
To run a KAGOME node, you need to provide it with a genesis config, cryptographic keys, and a place to store db files.
* Example of a genesis config file can be found in `examples/first_kagome_chain/localchain.json`
* Example of a base path dir can be found in `examples/first_kagome_chain/base_path`
* To create database files, just provide any base path into `kagome` executable (mind that start with authority role requires keys to start).


## Contributing Guides

Please refer to the [Contributor Documentation](./docs/source/development/dev-guide.md).

## Modules
* [api](./core/api)
    * JSON-RPC based on specification from [PSP](https://github.com/w3f/PSPs/blob/master/PSPs/drafts/psp-6.md) and Polkadot JS [documentation](https://polkadot.js.org/docs/substrate/rpc). Uses HTTP and WebSockets for transport. 
* [application](./core/application)
    * Implements logic for running KAGOME node, such as processing CLI flags and defining an execution order of different modules
* [assets](./core/assets)
    * Artifacts needed to run in development mode
* [authority_discovery](./core/authority_discovery)
    * Logic for finding peer information by authority id provided
* [authoriship](./core/authorship)
    * Mechanism for building blocks from extrinsics provided by the transaction pool
* [blockchain](./core/blockchain)
    * Implements blockchain logic such as fork handling and block digest information processing
* [clock](./core/clock)
    * Implements a clock interface that is used to access the system time
    * Implements timer interface that is used to schedule events
* [common](./core/common)
    * A set of miscellaneous primitives, data structures, and helpers that are widely used in the project to simplify development
* [consensus](./core/consensus)
    * Implementation of [BABE](https://spec.polkadot.network/#sect-block-production) block production mechanism
    * Implementation of [Grandpa](https://spec.polkadot.network/#sect-finality) finality gadget
* [containers](./core/containers)
    * An implementation of a container that serves as an allocator for frequently created objects
* [crypto](./core/crypto)
    * Crypto primitives implementation such as DSAs, Hashers, VRF, Random generators
* [filesystem](./core/filesystem)
    * A convenient interface for working with directories in the project
* [host_api](./core/host_api)
    * [Host APIs](https://spec.polkadot.network/#chap-host-api) exposed by the Polkadot runtime as WASM import functions needed to handle storage, cryptography, and memory
* [injector](./core/injector)
    * A mechanism for matching class interfaces to their corresponding implementations using the [Boost dependency injector](https://github.com/boost-ext/di)
* [log](./core/log)
    * A configuration of KAGOME logger
* [macro](./core/macro)
    * Convenience macros 
* [metrics](./core/metrics)
    * Prometheus metrics to retrieve KAGOME node execution statistics
* [network](./core/network)
    * A set of networking substream protocols implementation on top of [cpp-libp2p](https://github.com/libp2p/cpp-libp2p/) library
* [offchain](./core/offchain)
    * [Offchain worker](https://spec.polkadot.network/#sect-offchain-api) implementation
* [outcome](./core/outcome)
    * Convenient functions for [boost outcome](https://www.boost.org/doc/libs/1_81_0/libs/outcome/doc/html/index.html)
* [parachain](./core/parachain)
    * Parachains logic such as collation protocol, backing, availability and validity
* [runtime](./core/runtime)
    * Integration of Binaryen and WAVM WebAssembly engines with Host APIs
* [scale](./core/scale)
    * Scale codec for some primitives
* [storage](./core/storage)
    * Storage and trie interfaces with RocksDB implementation
* [subscription](./core/subscription)
    * Subscription engine
* [telemetry](./core/telemetry)
    * Connection to telemetry such as https://telemetry.polkadot.io/
* [transaction_pool](./core/transaction_pool)
    * Pool of transactions to be included into the block
* [utils](./core/utils)
    * Utils such as profiler, pruner, thread pool

You can find more information about the components by checking [reference documentation](https://kagome.netlify.com). Check out tutorials and more examples in official documentation: https://kagome.readthedocs.io/

## KAGOME in media

* Press-release: [Soramitsu to implement Polkadot Runtime Environment in C++](https://medium.com/web3foundation/w3f-grants-soramitsu-to-implement-polkadot-runtime-environment-in-c-cf3baa08cbe6)
* [KAGOME: C++ implementation of PRE](https://www.youtube.com/watch?v=181mk2xvBZ4&t=) presentation at DOTCon (18.08.19)
* [KAGOME and consensus in Polkadot](https://www.youtube.com/watch?v=5OrevTjaiPA) presentation (in Russian) during Innopolis blockchain meetup (28.10.19)
* [Web3 Builders: Soramitsu | C++ Implementation of Polkadot Host](https://www.youtube.com/watch?v=We3kiGzg60w) Polkadot's Web3 builders online presentation 
