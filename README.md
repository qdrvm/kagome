[//]: # (
Copyright Quadrivium LLC
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

![logo](/docs/image_assets/kagome-logo-(new-2020).svg)

[![](https://img.shields.io/twitter/follow/qdrvm_io?label=Follow&style=social)](https://twitter.com/qdrvm_io)

## Intro

KAGOME is a [Polkadot Host](https://github.com/w3f/polkadot-spec/tree/master/host-spec) (former Polkadot Runtime Environment) developed by [Quadrivium](https://qdrvm.io) and funded by a Web3 Foundation [grant](https://github.com/w3f/Web3-collaboration/blob/master/grants/grants.md) and Treasury proposals ( [1](https://kusama.polkassembly.io/post/1858), [2](https://polkadot.polkassembly.io/treasury/485)), [3](https://polkadot.polkassembly.io/treasury/876).


## Status

![kagome-components-Host drawio-light](https://github.com/qdrvm/kagome/assets/9370151/323c1677-b628-468c-adb8-5c09c2164fc3)


- [x] JSON-RPC (compatible with Polkadot JS)
    - [ ] New [unstable JSON-RPC](https://paritytech.github.io/json-rpc-interface-spec/)
- [x] Scale codec
- [x] Synchronizer
    - [x] Full sync
    - [x] Fast sync
    - [x] Warp sync
- [x] Transaction pool
- [x] Consensus
    - [x] BABE
    - [x] GRANDPA
- [x] Storage
    - [x] Blockchain
        - [x] Block storage
        - [x] Block tree
        - [x] Digest tracker
    - [x] Trie storage (merkle trie)
    - [x] RocksDB
    - [x] Dynamic pruning
    - [ ] Trie nodes caches
    - [ ] State caches
- [x] Runtime
    - [x] Host API
    - [x] WASM engine
        - [x] Binaryen
        - [x] WAVM
        - [x] WasmEdge
- [x] Parachains core
   - [x] Data availability
       - [x] Systematic chunks ([RFC-47](https://polkadot-fellows.github.io/RFCs/approved/0047-assignment-of-availability-chunks.html))
   - [x] Approval voting
   - [x] Disputes resolution
   - [x] Async-Backing
   - [ ] Elastic scaling
- [x] Networking
    - [x] Peer manager
        - [x] /dot/block-announces/1
        - [x] /paritytech/grandpa/1
        - [x] /polkadot/validation/1
        - [x] /polkadot/collation/1
        - [x] /dot/transactions/1
        - [x] /polkadot/req_collation/1
        - [x] /dot/light/2
        - [x] /polkadot/req_pov/1
        - [x] /dot/state/2
        - [x] /dot/sync/warp
        - [x] /polkadot/req_statement/1
        - [x] /dot/sync/2
        - [x] /polkadot/req_available_data/1
        - [x] /polkadot/req_chunk/1
        - [x] /polkadot/send_dispute/1
    - [x] Libp2p
        - [x] Transport
            - [x] TCP
            - [x] QUIC
            - [ ] WebRTC
        - [x] Secure connection
            - [x] Noise
            - [x] TLS
        - [x] Multiplexing
            - [x] Yamux
        - [x] Multiselect protocol
        - [x] Peer discovery
            - [x] Kademlia
        - [x] Ping protocol
        - [x] Identify protocol
- [x] Offchain workers
- [x] Keystore
- [x] Telemetry support
- [x] Prometheus metrics

More details of KAGOME development can be found within the [supported features](./README.md/#supported-features) section and in [projects board](https://github.com/soramitsu/kagome/projects/2)

## Getting Started

### Build

#### Prerequisites

If you are using a Debian Linux system, the following command allows you to build KAGOME:

```sh
git clone https://github.com/qdrvm/kagome
cd kagome
sudo chmod +x scripts/init.sh scripts/build.sh

sudo ./scripts/init.sh
./scripts/build.sh
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

### Installation from APT Package

To install KAGOME releases using the provided package repository, follow these steps (tested on Ubuntu 24.04.1 LTS (Noble Numbat)):

Update your package lists and install necessary utilities:
    
```sh
apt update && apt install -y gpg curl
```

Add the repository’s GPG signing key:
    
```sh
curl -fsSL https://europe-north1-apt.pkg.dev/doc/repo-signing-key.gpg | gpg --dearmor -o /usr/share/keyrings/europe-north-1-apt-archive-keyring.gpg
```

Add the KAGOME package repository to your sources list:
```sh
echo "deb [signed-by=/usr/share/keyrings/europe-north-1-apt-archive-keyring.gpg] https://europe-north1-apt.pkg.dev/projects/kagome-408211 kagome main" > /etc/apt/sources.list.d/kagome.list
```

Update the package lists and install KAGOME:

```sh
apt update && apt install -y kagome
```

### Using KAGOME

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


#### Execute KAGOME block_validator node in development mode

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

#### Running KAGOME as a Service

You can run KAGOME as a service using a systemd service file. Below is an example of a service file to launch KAGOME Kusama validator:
```sh
[Unit]
Description=Kagome Node

[Service]
User=kagome
Group=kagome
LimitCORE=infinity
LimitNOFILE=65536
ExecStart=kagome \ # should be in path
  --name kagome-validator \
  --base-path /home/kagome/dev/kagome-fun/kusama-node-1 \
  --public-addr=/ip4/212.11.12.32/tcp/30334 \ # Address should be publicly accessible
  --validator \
  --listen-addr=/ip4/0.0.0.0/tcp/30334 \
  --chain kusama \
  --prometheus-port=9615 \
  --prometheus-external \
  --wasm-execution Compiled \
  --telemetry-url 'wss://telemetry.polkadot.io/submit/ 1' \
  --rpc-port=9944 \
  --node-key 63808171009b35fc218f207442e355b0634561c84e0aec2093e3515113475624

Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

##### Adding the Service File

1. Copy the service file content into a new file named kagome.service.
2. Move the file to the systemd directory:

`sudo mv kagome.service /etc/systemd/system/`

##### Starting the Kagome Node

To start the Kagome node using systemd:
1. Reload the systemd manager configuration:
```sh
sudo systemctl daemon-reload
```

2. (Optionally) Enable the Kagome service to start on boot:
```sh
sudo systemctl enable kagome
```

3. Start the Kagome service:
```sh
sudo systemctl start kagome
```

4. Check the status of the Kagome service:
```sh
sudo systemctl status kagome
```

#### Run KAGOME with collator

Read [this](./examples/adder-collator) tutorial



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
* [authorship](./core/authorship)
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

You can find more information about the components by checking [reference documentation](https://kagome.netlify.com). Check out tutorials and more examples in the official documentation: https://kagome.readthedocs.io/

## KAGOME in media

* Press-release: [Soramitsu to implement Polkadot Runtime Environment in C++](https://medium.com/web3foundation/w3f-grants-soramitsu-to-implement-polkadot-runtime-environment-in-c-cf3baa08cbe6)
* [KAGOME: C++ implementation of PRE](https://www.youtube.com/watch?v=181mk2xvBZ4&t=) presentation at DOTCon (18.08.19)
* [KAGOME and consensus in Polkadot](https://www.youtube.com/watch?v=5OrevTjaiPA) presentation (in Russian) during Innopolis blockchain meetup (28.10.19)
* [Web3 Builders: Soramitsu | C++ Implementation of Polkadot Host](https://www.youtube.com/watch?v=We3kiGzg60w) Polkadot's Web3 builders online presentation
* [Building alternative clients](https://youtu.be/TnENz6I9l8A?si=yF4oo2zQ6LdPW13N) Polkadot Decoded 2023 presentation
* [Polkadot Host architecture in 2024](https://www.youtube.com/watch?v=Lv2KQ2EDyM8&pp=ygUKc3ViMCBrYW1pbA%3D%3D) Sub0 Asia 2024 presentation
* [Building Kagome: Polkadot's Journey Towards Client Diversity](https://youtu.be/tkem7nYd5Y0?feature=shared) AwesomeDOT Podcast
