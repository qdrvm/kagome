# Start private Kagome network

In this tutorial we will learn how to start a blockchain nework with a validator and block producing nodes

First go to tutorial's folder:

```bash
cd examples/network
```

## Execute validating node

First we execute validating node in the simillar way we did it during previous tutorial. This node will produce and finalize blocks.

To start with let's navigate into the node's folder:

```bash
kagome_validating \
    --genesis testchain.json \
    --keystore testkeystore1.json \
     --leveldb test_ldb1 \ 
     --p2p_port 11122 \
     --rpc_http_port 11133 \
     --rpc_ws_port 11144 \
     --genesis_epoch \
     -f
```

This time we also included `-f` flag to indicate that current node is the only one participating in finalization.

> At the moment Grandpa catch up logic is not implemented in Kagome, therefore we cannot execute more than a single finalizing node

You should get the following output if you did everything correctly:

```
[2020-06-06 20:30:49][info] Initialize trie storage with root: 03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314
[2020-06-06 20:30:49][info] Added block. Number: 0. Hash: cdfcad1790fdca062987d1f2b73bc506e2b24db18bf7b24a7f5b11d7954e9a33. State root: e457e36f0e2a60d1590a896b044b013648ecf2ac5b84ec83af0ba03abc99f603
[2020-06-06 20:30:49.426] [info] Grandpa authority: d046dde66d247e98e6c95366c05b6137ffeb61e9ee8541200569e70ac7632a46
[2020-06-06 20:30:49][info] Start as N6kagome11application25ValidatingNodeApplicationE with PID 29626
[2020-06-06 20:30:49][info] Started listening http session on port 11133
[2020-06-06 20:30:50][info] starting a slot 0 in epoch 0
[2020-06-06 20:30:50][info] Started listening with peer id: 12D3KooWE77U4m1d5mJxmU61AEsXewKjKnG7LiW2UQEPFnS6FXhv on address: /ip4/0.0.0.0/tcp/11122
[2020-06-06 20:30:52][info] starting a slot 1 in epoch 0
[2020-06-06 20:30:55][info] starting a slot 2 in epoch 0
...
```

## Execute block producing node

Block producing node can only participate in block production, but not in block finalization.  

Now that validating node is up and running, second node can join the network by bootstrapping from the first node. Command will look very simillar. Only this time we will use `kagome_block_producing` instead of `kagome_validating`:

```
kagome_block_producing \
    --genesis testchain.json \
    --keystore testkeystore2.json \
    --leveldb test_ldb2 \
    --p2p_port 11222 \
    --rpc_http_port 11233 \
    --rpc_ws_port 11244
```

Block producing node passes several steps before actual block production begins:

1. Waiting for block announcements to understand which blocks are missing.
2. Synchronize blocks between the latest synchronized one and the received one
3. Listen for several blocks, to figure out the slot time
4. Start block production when slot time is calculated using [median algorithm](https://research.web3.foundation/en/latest/polkadot/BABE/Babe.html#-4.-clock-adjustment--relative-time-algorithm-)

> Because these two nodes are running on the same machine, second node must be specified with different port numbers 

If you followed steps correctly block producing node will print the following logs:

```
[2020-06-06 20:33:13][info] Initialize trie storage with root: 03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314
[2020-06-06 20:33:13][info] Added block. Number: 0. Hash: cdfcad1790fdca062987d1f2b73bc506e2b24db18bf7b24a7f5b11d7954e9a33. State root: e457e36f0e2a60d1590a896b044b013648ecf2ac5b84ec83af0ba03abc99f603
[2020-06-06 20:33:13.340] [info] Grandpa authority: ba891366d247202ef7f88e81d49ef4c62bc5d23173e6b5fb0bca2817d856c980
[2020-06-06 20:33:13][info] Start as N6kagome11application29BlockProducingNodeApplicationE with PID 31295
[2020-06-06 20:33:13][info] Started listening http session on port 11233
[2020-06-06 20:33:13][info] Started listening with peer id: 12D3KooWKzGMnzpdQQFGvxVEBYKQuyttE3KjigvBAwXy7hZtdHqs on address: /ip4/0.0.0.0/tcp/11222
[2020-06-06 20:33:16][info] Received block announce: block number 4
[2020-06-06 20:33:16][info] Catching up to block number: 4
[2020-06-06 20:33:16][info] Requesting blocks from cdfcad1790fdca062987d1f2b73bc506e2b24db18bf7b24a7f5b11d7954e9a33 to 2d1589af5a7bd222da7e90b41680de0d9ae214476bab6f624839bfb59041793f
...
```

Note that both validating and block producing nodes had the same trie storage root. Second node received block announce for the block number 4 with hash `2d1589af5a7bd222da7e90b41680de0d9ae214476bab6f624839bfb59041793f` and started synchronization of the missing blocks.

## Execute syncing node

Syncing node cannot participate in either block production or block finalization. However, it can connect to the network and import all produced blocks. Besides that, syncing node can also receive extrinsics and broadcast them to the network.

To start syncing node `kagome_full_syncing` binary is used as follows:

```
kagome_full_syncing --genesis testchain.json -leveldb syncing_ldb1 --p2p_port 21122 --rpc_http_port 21133 --rpc_ws_port 21144
```

If everything is done correctly you should see the following output:

 ```

[2020-06-06 21:16:31][info] Initialize trie storage with root: 03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314
[2020-06-06 21:16:31][info] Added block. Number: 0. Hash: cdfcad1790fdca062987d1f2b73bc506e2b24db18bf7b24a7f5b11d7954e9a33. State root: e457e36f0e2a60d1590a896b044b013648ecf2ac5b84ec83af0ba03abc99f603
[2020-06-06 21:16:31.782] [info] Grandpa authority: 03d80edbdfb0316668dd57a530479ef79138e67a2e6f0cf545ec51fe30ea66f0
[2020-06-06 21:16:31.782] [info] Grandpa authority: 02a4e017293b86f3b74df2f3b033a9de02b6e5e36e7ca1b643c92eefb5cdc09d
[2020-06-06 21:16:31.782] [info] Grandpa authority: ba891366d247202ef7f88e81d49ef4c62bc5d23173e6b5fb0bca2817d856c980
[2020-06-06 21:16:31.782] [info] Grandpa authority: 56711b24c9ff377a11e515f09d5504abfa7247c964d55700893363e8bd54bf94
[2020-06-06 21:16:31][info] Start as N6kagome11application22SyncingNodeApplicationE with PID 32270
[2020-06-06 21:16:31][info] Started listening http session on port 21133
[2020-06-06 21:16:31][info] Started listening with peer id: 12D3KooWLDym5WmAoWfSt1gjrNQjrQrZqW4CX9TEyCbVixPuhFKP on address: /ip4/0.0.0.0/tcp/21122
[2020-06-06 21:16:34][info] Received block announce: block number 227
[2020-06-06 21:16:34][info] Received block header. Number: 227, Hash: 88245b415ef52978875d39d8d0aa9e3e2f966eb12c54f25147a949fbc88560be
[2020-06-06 21:16:34][info] Requesting blocks from cdfcad1790fdca062987d1f2b73bc506e2b24db18bf7b24a7f5b11d7954e9a33 to 88245b415ef52978875d39d8d0aa9e3e2f966eb12c54f25147a949fbc88560be
[2020-06-06 21:16:35][info] Received blocks from: cdfcad1790fdca062987d1f2b73bc506e2b24db18bf7b24a7f5b11d7954e9a33, to 88245b415ef52978875d39d8d0aa9e3e2f966eb12c54f25147a949fbc88560be
[2020-06-06 21:16:35][info] Applying block number: 1, hash: 491cbce4f99d88a26b572b49db165937558f0613c684be0de1b5762105afe641
```

Note that trie root is the same with validating and block producing nodes. When syncing node receives block announcement it first synchronizes missing blocks and then listens to the new blocks and finalization. 
