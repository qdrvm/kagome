[//]: # (
Copyright Quadrivium LLC
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Starting a Blockchain Network: A Step-by-Step Guide

In this tutorial, we will guide you through the process of setting up a blockchain network with both validating and syncing nodes.

## Step 1: Navigate to the Tutorial's Directory

Start by navigating to the directory containing the necessary resources for this tutorial.

```shell script
cd examples/network
```

## Step 2: Initialize the First Validating Node

In this step, we will launch the first validating node, similar to what we did in the previous tutorial. This node will be responsible for producing and finalizing blocks.

```shell script
kagome \
    --alice \
    --chain testchain.json \
    --base-path validating1 \
    --port 11122 \
    --rpc-port 11133 \
    --prometheus-port 11155
```

## Step 3: Bring the Second Validating Node Online

With the first validating node actively running, we can add the second validating node to the network. This node will bootstrap from the first one. The command to achieve this is very similar to the one we used for the first node.

```shell script
kagome \
    --bob \
    --chain testchain.json \
    --base-path validating2 \
    --port 11222 \
    --rpc-port 11233 \
    --prometheus-port 11255
```

Once activated, the second node performs several steps before beginning the actual block production:

1. **Block Announcements:** The node waits for block announcements to determine which blocks it's missing.
2. **Synchronization:** The node synchronizes blocks between the most recent one it has and the one it just received.
3. **Listening:** The node listens for several blocks to determine the timing for slotting.
4. **Production:** The node begins block production once it has calculated the slot time.

> Please note, because these two nodes are running on the same machine, it's crucial to use different port numbers for the second node to avoid conflicts.

### Step 4: Start syncing node

Syncing node cannot participate in block production or block finalization. However, it can connect to the network and import all produced blocks. Besides that, syncing node can also receive extrinsics and broadcast them to the network.In the world of blockchain, a syncing node plays a unique role. While it can't contribute to block production or finalization, it serves as a connector to the network, importing all produced blocks. More than that, a syncing node can receive extrinsics and subsequently broadcast them to the network.

To initiate a syncing node, you can use the binary `kagome` as outlined in the command below:

```shell script
kagome \
    --chain testchain.json \
    --base-path syncing1 \
    --port 21122 \
    --rpc-port 21133 \
    --prometheus-port 21155
```

Here's what happens when a syncing node receives a block announcement: it first compensates for any missing blocks, synchronizing them as required. Following this, it listens out for new blocks and finalization updates in the network. 

## Step 5: Initiate a Transaction

Recalling our previous tutorial, we will execute a fund transfer from Alice to Bob. This step is designed to confirm that every node in the network applies the transaction correctly.

You have the flexibility to send the transaction to any node. As soon as you do so, the transaction will be propagated to the block-producing nodes. From there, it remains stored in their transaction pools until it gets included in a block:

```shell script
# from kagome root directory
cd examples/transfer
python3 transfer.py localhost:21133 0xe5be9a5092b81bca64be81d212e7f2f9eba183bb7a90954f7b76361f6edb5c0a 5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty 2
```
