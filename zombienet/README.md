[//]: # (
Copyright Quadrivium LLC
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Zombienet tests

## Resources

* [zombienet repo](https://github.com/paritytech/zombienet)
* [zombienet book](https://paritytech.github.io/zombienet/)

## Running tests locally
To run any test locally use the native provider (`zombienet test -p native ...`) you need first build the binaries. They are:

* [polkadot-sdk](https://github.com/paritytech/polkadot-sdk/)

* polkadot-sdk/target/testnet/ -> adder-collator, malus, undying-collator 
* polkadot-sdk/target/release/ -> polkadot, polkadot-execute-worker, polkadot-prepare-worker, polkadot-parachain 

To build them use:
* adder-collator -> `cargo build --profile testnet -p test-parachain-adder-collator`
* undying-collator -> `cargo build --profile testnet -p test-parachain-undying-collator`
* malus -> `cargo build --profile testnet -p polkadot-test-malus`
* polkadot-parachain -> `cargo build --release -p polkadot-parachain-bin --bin polkadot-parachain`
  
You can run a test locally by executing:
```sh
zombienet test -p native 0001-parachains-smoke-test.zndsl
```

## Kubernetes as provider
Using kubernetes as provider we can simply spawn this network by running:

```sh
zombienet test -p kubernetes 0001-parachains-smoke-test.zndsl
```
or simpler, since kubernetes is the default provider as:

```sh
zombienet test 0001-parachains-smoke-test.zndsl
```

NOTE: Add correct images for tests in config(toml file) 
