# How to run rococo-local with parachains on KAGOME

> Note that for this tutorial, the rococo-local chainspec has been modified to make the epochs six minutes long. Original rococo-local chainspec could be obtained by typing `polkadot build-spec --chain rococo-local.json --raw --disable-default-bootnode > rococo-local.json`

> This tutorial is based on the [original tutorial](https://github.com/paritytech/polkadot/tree/1d05f779b25e01a1d54dbf98a82662d12a8320f9/parachain/test-parachains/adder/collator) from Polkadot repo


## Start kagome

### Run KAGOME network

Open two terminals and run a unique instance of KAGOME validator in each of them
```
kagome --chain rococo-local-raw.json --validator --alice --port 50551 --ws-port 9944 --node-key f6830f530f4fa0249357b56575162180f4c6822ee6e59e9081d8001cb200b66c --tmp
kagome --chain rococo-local-raw.json --validator --bob   --port 50552 --ws-port 9945 --node-key 15ed5bd565a9d0a3afb8f65a6b27795fef17247b03d0af1ce271f33504090e10 --tmp
```

### Run adder collator

First build `adder-collator` binary from [Polkadot](https://github.com/paritytech/polkadot) repo by running `cargo build --release`. Binary will appear in `target/release/` folder.

```
adder-collator --tmp --chain rococo-local-raw.json --port 50553 --ws-port 9947
```

Adder collator will print genesis state and wasm code which you should store in `genesis.state` and `adder.wasm` files correspondingly (or simply use files that are already in current folder). 

### Register parachain

1. Go to https://polkadot.js.org/apps/?rpc=ws%3A%2F%2F127.0.0.1%3A9944#/parachains/parathreads in your browser.
2. Add new para id using Alice account
3. Register parathread using `adder.wasm`and `genesis.state` as parameters
