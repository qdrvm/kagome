![logo](/docs/image_assets/kagome-logo-(new-2020).svg)

[![CodeFactor](https://www.codefactor.io/repository/github/soramitsu/kagome/badge)](https://www.codefactor.io/repository/github/soramitsu/kagome)
[![codecov](https://codecov.io/gh/soramitsu/kagome/branch/master/graph/badge.svg)](https://codecov.io/gh/soramitsu/kagome)
[![Netlify Status](https://api.netlify.com/api/v1/badges/ad6fa504-99d6-48fb-9a05-869ba1d9a7c3/deploy-status)](https://app.netlify.com/sites/kagome/deploys)
[![](https://img.shields.io/twitter/follow/Soramitsu_co?label=Follow&style=social)](https://twitter.com/Soramitsu_co)

## Intro

KAGOME is a [Polkadot Host](https://github.com/w3f/polkadot-spec/tree/master/host-spec) (former Polkadot Runtime Environment) developed by [Soramitsu](https://soramitsu.co.jp/) and funded by a Web3 Foundation [grant](https://github.com/w3f/Web3-collaboration/blob/master/grants/grants.md).


## Status

- [x] Syncing node
    - Polkadot, Kusama and Rococo compatibility
- [x] Validating node
- [x] Polkadot JS apps support
- [ ] Parachains support
- [x] telemetry support
- [ ] Fast sync
- [ ] Warp sync
- [ ] Light client

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

After downloading the snapshot you can unarchive it in the folder where the node will be running:

```
unzip polkadot-node-1.zip
```

#### Execute KAGOME Polkadot full syncing node

You can synchronize with Polkadot using KAGOME and obtain archive node that can be used to query Polkadot network at any state.

To launch KAGOME Polkadot syncing node execute:
```
cd examples/polkadot/
PATH=$PATH:../../build/node/
kagome --chain polkadot.json --base-path polkadot-node-1
```

After this command KAGOME will connect with other nodes in the network and start importing blocks. You may play with your local node using polkadot js apps: https://polkadot.js.org/apps/?rpc=ws%3A%2F%2F127.0.0.1%3A9944#/explorer

You will also be able to see your node on https://telemetry.polkadot.io/. If you need to identify it more easily you can add `--name <node-name>` flag to node's execution command and find your node in telemetry by typing its name.

Run `kagome --help` to explore other CLI flags.


#### Execute KAGOME validating node in development mode

The easiest way to get started with KAGOME is to execute in development mode which is a single node network:

```
kagome --dev
```

That executes node with default accounts Alice and Bob. You can read about these accounts [here](https://kagome.readthedocs.io/en/latest/tutorials/first_kagome_chain.html#launch-kagome-network).

To launch with wiping existing data you can do:

```
kagome --dev-with-wipe
```

#### Execute KAGOME node with validator mode

To launch KAGOME validator execute:
```
cd examples/first_kagome_chain
PATH=$PATH:../../build/node/
kagome --validator --chain localchain.json --base-path base_path
```

This command executes KAGOME full node with authority role.





### Configuration Details
To run a KAGOME node, you need to provide to it a genesis config, cryptographic keys and a place to store db files.
* Example of a genesis config file can be found in `examples/first_kagome_chain/localchain.json`
* Example of a base path dir can be found in `examples/first_kagome_chain/base_path`
* To create leveldb files, just provide any base path into `kagome` executable (mind that start with authority role requires keys to start).


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
    * KAGOME uses [cpp-libp2p](https://github.com/soramitsu/libp2p) for peer-to-peer interactions and peer discovery
    * Gossiper and Gossiper observer
    * SyncClient and SyncServer

You can find more information about the components by checking [reference documentation](https://kagome.netlify.com). Check out tutorials and more examples in official documentation: https://kagome.readthedocs.io/

## KAGOME in media

* Press-release: [Soramitsu to implement Polkadot Runtime Environment in C++](https://medium.com/web3foundation/w3f-grants-soramitsu-to-implement-polkadot-runtime-environment-in-c-cf3baa08cbe6)
* [KAGOME: C++ implementation of PRE](https://www.youtube.com/watch?v=181mk2xvBZ4&t=) presentation at DOTCon (18.08.19)
* [KAGOME and consensus in Polkadot](https://www.youtube.com/watch?v=5OrevTjaiPA) presentation (in Russian) during Innopolis blockchain meetup (28.10.19)
* [Web3 Builders: Soramitsu | C++ Implementation of Polkadot Host](https://www.youtube.com/watch?v=We3kiGzg60w) Polkadot's Web3 builders online presentation 
