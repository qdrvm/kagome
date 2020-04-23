# Tutorial

## Transfer

In this tutorial you will learn how to launch Kagome chain which can be used as a cryptocurrency, and interact with it by sending extrinsics and executing queries.

### Prerequisites

1. Kagome full node binary built as described [here](../overview/getting_started.md#Build full validating node).
2. For your convenience make sure you have this binary included into your path:
```
PATH=$PATH:build/node/kagome_full/
```
3. Python 3 installed in the system  

### Tutorial

For this tutorial we will spin up a simple network of a single peer with predefined genesis.

To start with let's navigate into the node's folder:

```
cd node
```

`node` folder contains necessary configuration files for our tutorial:

1. `config/localchain.json` – genesis file for our network. It contains necessary key-value pairs that should be inserted before the genesis block.
2. `config/localkeystore.json` – file containing the keys for Kagome peer. This is necessary to be able to sign the messages sent by our validating node. 

---
**Note**

If you are running this tutorial not for the first time, then make sure you cleaned up your storage as follows:
```
rm -rf ldb
```
--- 

To execute the chain run the following command:

`kagome_full --genesis config/localchain.json --keystore config/localkeystore.json -l ldb -e`

You should see the log messages about producing and finalizing the blocks. 

---
**Note**

Kagome blockchain is constantly producing new blocks, even if there were no transactions in the network.

--- 

Now open second terminal and go to the examples folder

`cd examples/`

This folder contains two python scripts:

1. `alice_balance.py` – executes query to kagome, which returns balance of Alice's account
2. `transfer.py` – sends transaction, that transfers [??] amount of money from Alice to Bob

Let's query current balance of Alice's account:

```
python3 alice_balance.py
```
