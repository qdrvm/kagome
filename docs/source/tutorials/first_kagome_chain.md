[//]: # (
Copyright Quadrivium LLC
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

## Your first Kagome chain

In this comprehensive guide, we'll walk you through the process of setting up and running a Kagome-based Polkadot-host chain. This chain can function as a cryptocurrency. Additionally, we will demonstrate how to communicate with this chain through actions such as dispatching extrinsics and executing queries.

### Prerequisites

1. Kagome validating node binary built as described [here](https://kagome.readthedocs.io/en/latest/overview/getting_started.html#build-application).
2. For your convenience make sure you have this binary included into your path:

    ```bash
    # from Kagome's root repo:
    PATH=$PATH:$(pwd)/build/node/
    ```
   
3. Python 3 installed in the system with `substrate-interface` package installed

    > If you are not sure if you have requests package installed in your python run `pip3 install substrate-interface`

### Tutorial

In this tutorial, we will:
1. Launch the Kagome network using a prepared genesis file.
2. Query the balance of Alice's account.
3. Send an extrinsic that transfers a specific amount of currency from Alice to Bob.
4. Query Alice's balance again to confirm that it has been updated.

#### Launch Kagome network

In this tutorial, we will setup a basic network consisting of a single peer with a predefined genesis.

Firstly, we need to move to the node's directory:

```shell script
cd examples/first_kagome_chain
```

`first_kagome_chain` folder contains necessary configuration files for our tutorial:

* `localchain.json` – This is the genesis file for our network. It houses the crucial key-value pairs that need to be inserted before the creation of the genesis block.
* `base_path` – This directory encompasses the base path of the Kagome project. Inside, you'll find the 'chains' folder, which contains several directories. Each of these directories carries the name of the chain id it stores the data for (`rococo-dev` in this case). The data for each chain is comprised of `db/` (which gets initialized upon node startup) and `keystore/` (which contains the keys used to sign messages sent by our authority). It's important to note that the `keystore/` must exist before the node starts. However, there's an exception for predefined accounts such as Alice, Bob, etc.

`localchain.json` contains Alice and Bob accounts. Both have 999998900.0 amount of crypto.
Their keys can be generated using [subkey](https://substrate.dev/docs/en/knowledgebase/integrate/subkey) tool:
```shell script
subkey inspect //Alice
# Secret Key URI `//Alice` is account:
# Secret seed:      0xe5be9a5092b81bca64be81d212e7f2f9eba183bb7a90954f7b76361f6edb5c0a
# Public key (hex): 0xd43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d
# Account ID:       0xd43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d
# SS58 Address:     5GrwvaEF5zXb26Fz9rcQpDWS57CtERHpNehXCPcNoHGKutQY

subkey inspect //Bob  
# Secret Key URI `//Bob` is account:
# Secret seed:      0x398f0c28f98885e046333d4a41c19cee4c37368a9832c6502f6cfd182e2aef89
# Public key (hex): 0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48
# Account ID:       0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48
# SS58 Address:     5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
```


> If you are running this tutorial not for the first time, then make sure you cleaned up your storage as follows (assuming storage files are generated in ldb/ folder):
> ```
> rm -rf base_path/chains/rococo_dev/db
> ```
> ---

For this tutorial, you can start a single-node network as follows:

```shell script
kagome \
    --alice \
    --chain localchain.json \
    --base-path base_path \
    --port 30363 \
    --rpc-port 9933 \
    --ws-port 9944
```

Let's look at this flags in detail:

Let's examine these flags in detail:

| Flag          | Requirement | Description                              |
|---------------|-------------|------------------------------------------|
| `--alice`     | Optional    | Enables validator mode with Alice's keys |
| `--chain`     | Mandatory   | Specifies the chainspec file path        |
| `--base-path` | Mandatory   | Specifies the base Kagome directory path |
| `--port`      | Optional    | Sets the port for p2p interactions       |
| `--rpc-port`  | Optional    | Sets the port for RPC over HTTP and WS   |

It's important to note that the `--alice` flag is equivalent to the `--validator` flag with Alice's keys added to `base_path/chains/rococo_dev/keystore` directory. This implies that for this tutorial it's possible to initiate our node with an empty keystore since we'll be utilizing Alice's predefined keys. This will not be the case when our node is used as validator in the real network.

You can obtain in-depth information regarding the available flags by executing `kagome --help`.

Upon successful setup, you should observe log messages indicating that the blocks have been produced and finalized.

At present, the chain is running on a single node. To make queries to this setup, you can use the localhost's port 9933, which is designed for http- or websockets-based RPCs.

> Kagome blockchain is constantly producing new blocks, even if there were no transactions in the network.

#### Query the balance

Now, open a second terminal and navigate to the `transfer` folder, which is available from the root directory of the project.

`cd examples/transfer`

This folder contains two python scripts:

1. `balance.py <address> <account_id>` – This script executes a query to Kagome, which returns the balance of the provided account.
    * `<address>`: The address of the node's HTTP service
    * `<account_id>`:  The ID of the account to be queried.
2. `transfer.py <address> <seed> <dest> <amount>` – This script sends the provided extrinsic.
    * `<address>`: The address of the node's HTTP service.
    * `<seed>`: The secret seed of the source account.
    * `<dest>`>: The destination account.
    * `<amount>`>: The amount of cryptocurrency to be transferred.



Let's query the current balance of Alice's account.

```shell script
python3 balance.py localhost:9933 5GrwvaEF5zXb26Fz9rcQpDWS57CtERHpNehXCPcNoHGKutQY
# Current free balance: 10000.0  
```

Let's do the same for Bob's account.
```shell script
python3 balance.py localhost:9933 5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
# Current free balance: 10000.0  
```

#### Send extrinsic

We can generate extrinsic using the following command:

This command will create extrinsic that transfers 1 from Alice to Bob's account (Alice's account is defined by secret seed and Bob's account by account id).

To send extrinsic use `transfer.py` script as follows:
```shell script
python3 transfer.py localhost:9933 0xe5be9a5092b81bca64be81d212e7f2f9eba183bb7a90954f7b76361f6edb5c0a 5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty 1
Extrinsic '0x315060176633a3e20656bd15c6eceff63b9f8bdc45595dc8ad52a02ae38d1708' sent
```

#### Query again the balances

Now let's check that extrinsic was actually applied:

Get the balance of Bob's account:

```shell script
python3 balance.py localhost:9933 5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
# Current free balance: 10001.0  
```
We can see that Bob's balance was increased by 1 as it was set on the subkey command

Now let's check Alice's account:
```shell script
python3 balance.py localhost:9933 5GrwvaEF5zXb26Fz9rcQpDWS57CtERHpNehXCPcNoHGKutQY
Current free balance: 9998.999829217584  
```

We can see that Alice's account was decreased by more than 1. This is caused by the commission paid for transfer

### Conclusion

Now you know how to set up single peer kagome network and execute transactions on it
