![logo](/docs/image_assets/logo.png)

# Kagome
[![Build Status](https://travis-ci.org/soramitsu/kagome.svg?branch=master)](https://travis-ci.org/soramitsu/kagome)
[![code inspector](https://www.code-inspector.com/project/74/status/svg)](https://www.code-inspector.com/public/project/74/kagome/dashboard)
[![CodeFactor](https://www.codefactor.io/repository/github/soramitsu/kagome/badge)](https://www.codefactor.io/repository/github/soramitsu/kagome)
[![codecov](https://codecov.io/gh/soramitsu/kagome/branch/master/graph/badge.svg)](https://codecov.io/gh/soramitsu/kagome)
[![Netlify Status](https://api.netlify.com/api/v1/badges/ad6fa504-99d6-48fb-9a05-869ba1d9a7c3/deploy-status)](https://app.netlify.com/sites/kagome/deploys)
[![](https://img.shields.io/twitter/follow/espadrine.svg?label=Follow&style=social)](https://twitter.com/Soramitsu_co)

Kagome is a [Polkadot Runtime Environment](https://github.com/w3f/polkadot-spec/tree/master/runtime-environment-spec) developed by [Soramitsu](https://soramitsu.co.jp/) add funded by Web3 Foundation [grant](https://github.com/w3f/Web3-collaboration/blob/master/grants/grants.md). 

## Supported features
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
    * Merkle patricia trie implementation, described in spec
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


## Dev Guides

1. [Terms](./docs/terms.md)
2. [Rules](./docs/rules.md)
3. [Development guide](./docs/dev-guide.md)
4. [`outcome::result<T>` docs](./docs/result.md)
5. [Tooling](./docs/tooling.md)

## Dev Environment

1. `git clone https://github.com/soramitsu/polkadot`
2. `cd polkadot`
3. `git submodule update --init --recursive`
