## Your first Kagome chain

In this tutorial you will learn how to execute Kagome-based Polkadot-host chain which can be used as a cryptocurrency, and interact with it by sending extrinsics and executing queries.

### Prerequisites

1. Kagome validating node binary built as described [here](https://kagome.readthedocs.io/en/latest/overview/getting_started.html#build-full-validating-node).
2. For your convenience make sure you have this binary included into your path:
```
PATH=$PATH:build/node/kagome_validating/
```
3. Python 3 installed in the system  

### Tutorial

During this tutorial we will:
1. Launch Kagome network using prepared genesis file
2. Query the balance of Alice account
3. Send extrinsic that transfers some amount of currency from Alice to Bob
4. Query again the balance of Alice to make sure balance was updated

#### Launch Kagome network

For this tutorial we will spin up a simple network of a single peer with predefined genesis.

To start with let's navigate into the node's folder:

```
cd node
```

`node` folder contains necessary configuration files for our tutorial:

* `config/localchain.json` – genesis file for our network. It contains necessary key-value pairs that should be inserted before the genesis block    |
* `config/localkeystore.json` – file containing the keys for Kagome peer. This is necessary to be able to sign the messages sent by our validating node | 

localchain.json contains Alice and Bob accounts. Both have 1000000000000000000000 amount of crypto.
Their keys can be generated using [subkey](https://substrate.dev/docs/en/ecosystem/subkey) tool:
```
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
> rm -rf ldb
> ```
> ---

For this tutorial you can start a single node network as follows:

```
kagome_validating \
    --genesis config/localchain.json \
    --keystore config/localkeystore.json \
    --leveldb ldb \
    --rpc_http_port 40363 \
    --rpc_ws_port 40364
```

Let's look at this flags in detail:

* `--genesis` -– mandatory, genesis configuration file path
* `--keystore` –- mandatory, keystore file path
* `--leveldb` –- mandatory, leveldb directory path                 
* `--rpc_http_port` –- port for RPC over HTTP                            
* `--rpc_ws_port`   –- port for RPC over Websocket protocol              

More flags info available by running `kagome_validating --help`.

You should see the log messages notifying about produced and finalized the blocks. 

Now chain is running on a single node. To query it we can use localhost's ports 40363 for http- and 40364 for websockets-based RPCs.

> Kagome blockchain is constantly producing new blocks, even if there were no transactions in the network.

#### Query the balance

Now open second terminal and go to the examples folder, located in the projects root directory.

`cd examples/`

This folder contains two python scripts:

1. `balance.py <address> <balance_key>` – executes query to kagome, which returns balance of provided account
    * `<address>` address of the node
    * `<balance_key>` key of the account's balance. Follow [this](https://www.shawntabrizi.com/substrate/querying-substrate-storage-via-rpc/#storage-map-query) tutorial to find out how to generate such keys
2. `transfer.py <address> <extrinsic>` – sends provided extrinsic
    * `<address>` address of the node
    * `<extrinsic>` extrinsic to be executed on a full node



Let's query current balance of Alice's account. This balance is stored under `0x7f864e18e3dd8b58386310d2fe0919eef27c6e558564b7f67f22d99d20f587bb` key.

```bash
python3 balance.py localhost:40363 0x7f864e18e3dd8b58386310d2fe0919eef27c6e558564b7f67f22d99d20f587bb
# Balance is 1000000000000000000000  
```

Let's do the same for the Bob's account. His balance key is `0xaa91c89bc225912c164c982c289e170c5a0d0f8802b37d68f88e2821facf4b55`
```bash
python3 balance.py localhost:40363 0xaa91c89bc225912c164c982c289e170c5a0d0f8802b37d68f88e2821facf4b55
# Balance is 1000000000000000000000  
```

#### Send extrinsic

We can generate extrinsic using the following command:

```bash
subkey transfer -g b86008325a917cd553b122702d1346bf6f132db4ea17155a9eec733413dc9ecf 0xe5be9a5092b81bca64be81d212e7f2f9eba183bb7a90954f7b76361f6edb5c0a 0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48 1000 0
Using a genesis hash of b86008325a917cd553b122702d1346bf6f132db4ea17155a9eec733413dc9ecf
# 0x2d0284ffd43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d01f40a68108bf61df0e9d0108ab8b621b354d233067514055fc77542aa84b647608335134d45c4b3040b8c2830217aa8350091774eaf3c22644d8e0c8db54143860000000600ff8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48a10f
```

This command will create extrinsic that transfers 1000 from Alice to Bob's account (Alice's account is defined by secret seed and Bob's account by account id). To define on which blockchain this extrinsic is going to be executed we provide subkey with genesis hash `b86008325a917cd553b122702d1346bf6f132db4ea17155a9eec733413dc9ecf` which corresponds to the hash of the genesis block of our chain.

To send extrinsic use `transfer.py` script as follows:
```bash
python3 transfer.py localhost:40363 0x2d0284ffd43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d01f40a68108bf61df0e9d0108ab8b621b354d233067514055fc77542aa84b647608335134d45c4b3040b8c2830217aa8350091774eaf3c22644d8e0c8db54143860000000600ff8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48a10f
# Extrinsic submitted. Response:  {'jsonrpc': '2.0', 'id': 1, 'result': [39, 212, 157, 212, 66, 199, 109, 255, 180, 146, 47, 243, 118, 221, 233, 172, 35, 201, 157, 96, 248, 24, 22, 14, 230, 108, 217, 211, 29, 216, 65, 255]} 
```

#### Query again the balances

Now let's check that extrinsic was actually applied:

Get the balance of Bob's account:

```bash
python3 balance.py localhost:40363 0xaa91c89bc225912c164c982c289e170c5a0d0f8802b37d68f88e2821facf4b55
# Balance is 1000000000000000001000
```
We can see that Bob's balance was increased by 1000 as it was set on the subkey command

Now let's check Alice's account:
```bash
python3 balance.py localhost:40363 0x7f864e18e3dd8b58386310d2fe0919eef27c6e558564b7f67f22d99d20f587bb
# Balance is 999999996589072329000
```

We can see that Alice's account was decreased by more than 1000. This is caused by the commission paid for transfer

### Conclusion

Now you know how to set up single peer kagome network and execute transactions on it
