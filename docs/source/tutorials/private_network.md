## Start private Kagome network

In this tutorial we will learn how to start a blockchain network with a validator and block producing nodes

First go to tutorial's folder:

```shell script
cd examples/network
```

### Execute first validating node

First we execute validating node in the similar way we did it during previous tutorial. This node will produce and finalize blocks.

To start with let's navigate into the node's folder:

```shell script
kagome \
    --validator \
    --chain testchain.json \
    --base-path validating1 \
    --port 11122 \
    --rpc-port 11133 \
    --ws-port 11144 \
    --already-synchronized
```

### Execute second validating node (node with authority) 

Now that validating node is up and running, second node can join the network by bootstrapping from the first node. Command will look very similar.

```shell script
kagome \
    --validator \
    --chain testchain.json \
    --base-path validating2 \
    --port 11222 \
    --rpc-port 11233 \
    --ws-port 11244
```

Second node passes several steps before actual block production begins:

1. Waiting for block announcements to understand which blocks are missing.
2. Synchronize blocks between the latest synchronized one and the received one
3. Listen for several blocks, to figure out the slot time
4. Start block production when slot time is calculated using [median algorithm](https://research.web3.foundation/en/latest/polkadot/BABE/Babe.html#-4.-clock-adjustment--relative-time-algorithm-)

> Because these two nodes are running on the same machine, second node must be specified with different port numbers 

Note that both nodes have the same hash of block 0: `2b32173d63796278d1cea23fcb255866153f07700226f3d7ba348e25ae7f9d29`

### Execute syncing (without authority) node

Syncing node cannot participate in either block production or block finalization. However, it can connect to the network and import all produced blocks. Besides that, syncing node can also receive extrinsics and broadcast them to the network.

To start syncing node `kagome` binary is used as follows:

```shell script
kagome \
    --chain testchain.json \
    --base-path syncing1 \
    --port 21122 \
    --rpc-port 21133 \
    --ws-port 21144
```

Note that trie root is the same with validating nodes. When syncing node receives block announcement it first synchronizes missing blocks and then listens to the new blocks and finalization. 

### Send transaction

Like in previous tutorial we will send transfer from Alice to Bob to check that transaction was applied on every node.

We can send transaction on any of the node, as it will be propagated to the block producing nodes and stored in their transaction pools until transactions is included to the block:

```shell script
# from kagome root directory
cd examples/transfer
python3 transfer.py localhost:9933 0xe5be9a5092b81bca64be81d212e7f2f9eba183bb7a90954f7b76361f6edb5c0a 5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty 2
```
