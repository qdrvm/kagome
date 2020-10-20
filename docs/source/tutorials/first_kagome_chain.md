## Your first Kagome chain

In this tutorial you will learn how to execute Kagome-based Polkadot-host chain which can be used as a cryptocurrency, and interact with it by sending extrinsics and executing queries.

### Prerequisites

1. Kagome validating node binary built as described [here](https://kagome.readthedocs.io/en/latest/overview/getting_started.html#build-full-validating-node).
2. For your convenience make sure you have this binary included into your path:

    ```bash
    # from Kagome's root repo:
    PATH=$PATH:$(pwd)/build/node/kagome_validating/
    ```
   
3. Python 3 installed in the system with `requests` and `scalecodec` package installed

    > If you are not sure if you have requests package installed in your python run `pip3 install substrate-interface`

### Tutorial

During this tutorial we will:
1. Launch Kagome network using prepared genesis file
2. Query the balance of Alice account
3. Send extrinsic that transfers some amount of currency from Alice to Bob
4. Query again the balance of Alice to make sure balance was updated

#### Launch Kagome network

For this tutorial we will spin up a simple network of a single peer with predefined genesis.

To start with let's navigate into the node's folder:

```bash
cd examples/first_kagome_chain
```

`first_kagome_chain` folder contains necessary configuration files for our tutorial:

* `localchain.json` – genesis file for our network. It contains necessary key-value pairs that should be inserted before the genesis block    |
* `localkeystore.json` – file containing the keys for Kagome peer. This is necessary to be able to sign the messages sent by our validating node | 

`localchain.json` contains Alice and Bob accounts. Both have 1000000000000000000000 amount of crypto.
Their keys can be generated using [subkey](https://substrate.dev/docs/en/knowledgebase/integrate/subkey) tool:
```bash
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

```bash
kagome_validating \
    --genesis localchain.json \
    --keystore localkeystore.json \
    --leveldb ldb \
    --p2p_port 30363 \
    --rpc_http_port 9933 \
    --rpc_ws_port 9944 \
    -s \
    -f
```

Let's look at this flags in detail:

| Flag              | Description                                       |
|-------------------|---------------------------------------------------|
| `--genesis`       | mandatory, genesis configuration file path        |
| `--keystore`      | mandatory, keystore file path                     |
| `--leveldb`       | mandatory, leveldb directory path                 |
| `--p2p_port`      | port for p2p interactions                         |
| `--rpc_http_port` | port for RPC over HTTP                            |
| `--rpc_ws_port`   | port for RPC over Websocket protocol              |
| `--single_finalizing_node`   | need to be set if this is the only finalizing node              |
| `--already_synchronized`   | need to be set if need to be considered synchronized              |

More flags info available by running `kagome_validating --help`.

You should see the log messages notifying about produced and finalized the blocks. 

Now chain is running on a single node. To query it we can use localhost's ports 9933 for http- and 9944 for websockets-based RPCs.

> Kagome blockchain is constantly producing new blocks, even if there were no transactions in the network.

#### Query the balance

Now open second terminal and go to the transfer folder, located in the projects root directory.

`cd examples/transfer`

This folder contains two python scripts:

1. `balance.py <address> <balance_key>` – executes query to kagome, which returns balance of provided account
    * `<address>` address node's http service
    * `<account_id>` id of account being queried
2. `transfer.py <address> <extrinsic>` – sends provided extrinsic
    * `<address>` address node's http service
    * `<seed>` secret seed of source account
    * `<dest>` destination account
    * `<amount>` amount of crypto to be transferred



Let's query current balance of Alice's account.

```bash
python balance.py localhost:9933 5GrwvaEF5zXb26Fz9rcQpDWS57CtERHpNehXCPcNoHGKutQY
# Current free balance: 999998897.549684  
```

Let's do the same for the Bob's account.
```bash
python balance.py localhost:9933 5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
# Current free balance: 999998901.0  
```

#### Send extrinsic

We can generate extrinsic using the following command:

This command will create extrinsic that transfers 1000 from Alice to Bob's account (Alice's account is defined by secret seed and Bob's account by account id). To define on which blockchain this extrinsic is going to be executed we provide subkey with genesis hash `b86008325a917cd553b122702d1346bf6f132db4ea17155a9eec733413dc9ecf` which corresponds to the hash of the genesis block of our chain.

To send extrinsic use `transfer.py` script as follows:
```bash
python3 transfer.py localhost:9933 0x2d0284ffd43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d01f40a68108bf61df0e9d0108ab8b621b354d233067514055fc77542aa84b647608335134d45c4b3040b8c2830217aa8350091774eaf3c22644d8e0c8db54143860000000600ff8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48a10f 1
# Extrinsic submitted. Response:  {'jsonrpc': '2.0', 'id': 1, 'result': [39, 212, 157, 212, 66, 199, 109, 255, 180, 146, 47, 243, 118, 221, 233, 172, 35, 201, 157, 96, 248, 24, 22, 14, 230, 108, 217, 211, 29, 216, 65, 255]} 
```

#### Query again the balances

Now let's check that extrinsic was actually applied:

Get the balance of Bob's account:

```bash
python balance.py localhost:9933 5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
# Current free balance: 999998902.0  
```
We can see that Bob's balance was increased by 1 as it was set on the subkey command

Now let's check Alice's account:
```bash
python balance.py localhost:9933 5GrwvaEF5zXb26Fz9rcQpDWS57CtERHpNehXCPcNoHGKutQY
Current free balance: 999998895.0993682  
```

We can see that Alice's account was decreased by more than 1. This is caused by the commission paid for transfer

### Conclusion

Now you know how to set up single peer kagome network and execute transactions on it
