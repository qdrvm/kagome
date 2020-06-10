## Start private Kagome network

In this tutorial we will learn how to start a blockchain nework with a validator and block producing nodes

First go to tutorial's folder:

```bash
cd examples/network
```

### Execute validating node

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
     -f
```

This time we also included `-f` flag to indicate that current node is the only one participating in finalization.

> At the moment Grandpa catch up logic is not implemented in Kagome, therefore we cannot execute more than a single finalizing node

You should get the following output if you did everything correctly:

```
[2020-06-10 14:53:33][info] Initialize trie storage with root: 03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314
[2020-06-10 14:53:33][info] Added block. Number: 0. Hash: a445da57787222aaaed37bfdca0f64981443f9554865a43abb39c5c868ff8c96. State root: ae7c1e51060fb97f38ff262e2620d926f69569ce020510d971115bdaf3b62087
[2020-06-10 14:53:33.833] [info] Grandpa authority: d046dde66d247e98e6c95366c05b6137ffeb61e9ee8541200569e70ac7632a46
[2020-06-10 14:53:34][info] Start as N6kagome11application25ValidatingNodeApplicationE with PID 80596
[2020-06-10 14:53:34][info] Started listening http session on port 11133
[2020-06-10 14:53:34][info] starting a slot 0 in epoch 0
[2020-06-10 14:53:34][info] Started listening with peer id: 12D3KooWE77U4m1d5mJxmU61AEsXewKjKnG7LiW2UQEPFnS6FXhv on address: /ip4/0.0.0.0/tcp/11122
[2020-06-10 14:53:37][info] starting a slot 1 in epoch 0
[2020-06-10 14:53:40][info] starting a slot 2 in epoch 0
[2020-06-10 14:53:43][info] starting a slot 3 in epoch 0
[2020-06-10 14:53:46][info] starting a slot 4 in epoch 0
[2020-06-10 14:53:49][info] starting a slot 5 in epoch 0
[2020-06-10 14:53:52][info] starting a slot 6 in epoch 0
...
```

### Execute block producing node

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
[2020-06-10 14:54:42][info] Initialize trie storage with root: 03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314
[2020-06-10 14:54:42][info] Added block. Number: 0. Hash: a445da57787222aaaed37bfdca0f64981443f9554865a43abb39c5c868ff8c96. State root: ae7c1e51060fb97f38ff262e2620d926f69569ce020510d971115bdaf3b62087
[2020-06-10 14:54:42.856] [info] Grandpa authority: ba891366d247202ef7f88e81d49ef4c62bc5d23173e6b5fb0bca2817d856c980
[2020-06-10 14:54:43][info] Start as N6kagome11application29BlockProducingNodeApplicationE with PID 81192
[2020-06-10 14:54:43][info] Started listening http session on port 11233
[2020-06-10 14:54:43][info] Started listening with peer id: 12D3KooWKzGMnzpdQQFGvxVEBYKQuyttE3KjigvBAwXy7hZtdHqs on address: /ip4/0.0.0.0/tcp/11222
[2020-06-10 14:54:58][info] Received block announce: block number 4
[2020-06-10 14:54:58][info] Catching up to block number: 4
[2020-06-10 14:54:58][info] Requesting blocks from a445da57787222aaaed37bfdca0f64981443f9554865a43abb39c5c868ff8c96 to 4b34a6e9bed6330227763673b8ac0f28771f770648b96d1048ee7cea00fad8fe
[2020-06-10 14:54:58][info] Received blocks from: a445da57787222aaaed37bfdca0f64981443f9554865a43abb39c5c868ff8c96, to 4b34a6e9bed6330227763673b8ac0f28771f770648b96d1048ee7cea00fad8fe
[2020-06-10 14:54:58][info] Applying block number: 1, hash: 28d20a1144d0183276c4577d478311880eff7298eecea5aeb2175408084901f5
[2020-06-10 14:54:58][warning] Resetting state to: ae7c1e51060fb97f38ff262e2620d926f69569ce020510d971115bdaf3b62087
[2020-06-10 14:54:58][info] Added block. Number: 1. Hash: 28d20a1144d0183276c4577d478311880eff7298eecea5aeb2175408084901f5. State root: feae7d10521e2737c82b56307cec9c61dfc7804af7980f1d68adc144eb85b3bc
[2020-06-10 14:54:58][info] Imported block with number: 1, hash: 28d20a1144d0183276c4577d478311880eff7298eecea5aeb2175408084901f5
[2020-06-10 14:54:58][info] Applying block number: 2, hash: 20d7bc296eb54d22fd35bab15228e4fe63a16a98a984a9f4d245b7277259c56e
[2020-06-10 14:54:58][warning] Resetting state to: feae7d10521e2737c82b56307cec9c61dfc7804af7980f1d68adc144eb85b3bc
[2020-06-10 14:54:58][info] Added block. Number: 2. Hash: 20d7bc296eb54d22fd35bab15228e4fe63a16a98a984a9f4d245b7277259c56e. State root: 8727457a1ec775acc004943c601c9a23a863c604026eba0e9e0e013e2df77326
[2020-06-10 14:54:58][info] Imported block with number: 2, hash: 20d7bc296eb54d22fd35bab15228e4fe63a16a98a984a9f4d245b7277259c56e
[2020-06-10 14:54:58][info] Applying block number: 3, hash: b747ce845e5b545f3364388d14db6b46c680f8f178c4e95e81668ac12a91e04a
[2020-06-10 14:54:58][warning] Resetting state to: 8727457a1ec775acc004943c601c9a23a863c604026eba0e9e0e013e2df77326
[2020-06-10 14:54:58][info] Added block. Number: 3. Hash: b747ce845e5b545f3364388d14db6b46c680f8f178c4e95e81668ac12a91e04a. State root: 1261fcf37396fa17c6e4af82cb4e3b77b79b256831c158f10b5db52c7dd00aed
...
```

Note that both validating and block producing nodes had the same trie storage root after block 0: `ae7c1e51060fb97f38ff262e2620d926f69569ce020510d971115bdaf3b62087`. Block producing node received block announce for the block number 4 with hash `4b34a6e9bed6330227763673b8ac0f28771f770648b96d1048ee7cea00fad8fe` and started synchronization of the missing blocks.

### Execute syncing node

Syncing node cannot participate in either block production or block finalization. However, it can connect to the network and import all produced blocks. Besides that, syncing node can also receive extrinsics and broadcast them to the network.

To start syncing node `kagome_full_syncing` binary is used as follows:

```
kagome_full_syncing --genesis testchain.json -leveldb syncing_ldb1 --p2p_port 21122 --rpc_http_port 21133 --rpc_ws_port 21144
```

If everything is done correctly you should see the following output:

 ```
[2020-06-10 15:10:15][info] Initialize trie storage with root: 03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314
[2020-06-10 15:10:15][info] Added block. Number: 0. Hash: a445da57787222aaaed37bfdca0f64981443f9554865a43abb39c5c868ff8c96. State root: ae7c1e51060fb97f38ff262e2620d926f69569ce020510d971115bdaf3b62087
[2020-06-10 15:10:16.110] [info] Grandpa authority: 03d80edbdfb0316668dd57a530479ef79138e67a2e6f0cf545ec51fe30ea66f0
[2020-06-10 15:10:16.110] [info] Grandpa authority: 02a4e017293b86f3b74df2f3b033a9de02b6e5e36e7ca1b643c92eefb5cdc09d
[2020-06-10 15:10:16.110] [info] Grandpa authority: ba891366d247202ef7f88e81d49ef4c62bc5d23173e6b5fb0bca2817d856c980
[2020-06-10 15:10:16.110] [info] Grandpa authority: 56711b24c9ff377a11e515f09d5504abfa7247c964d55700893363e8bd54bf94
[2020-06-10 15:10:16][info] Start as N6kagome11application22SyncingNodeApplicationE with PID 83161
[2020-06-10 15:10:16][info] Started listening http session on port 21133
[2020-06-10 15:10:16][info] Started listening with peer id: 12D3KooW9yQ4WxF6VTQfQ5dNTg3jhvzJLdGpcyq9GAxwXmiCtaQG on address: /ip4/0.0.0.0/tcp/21122
[2020-06-10 15:10:25][info] Received block announce: block number 123
[2020-06-10 15:10:25][info] Received block header. Number: 123, Hash: 06cd05b69319a2c018e63db4dc30415824dc92599bf9b26487639d03fd6d4f66
[2020-06-10 15:10:25][info] Requesting blocks from a445da57787222aaaed37bfdca0f64981443f9554865a43abb39c5c868ff8c96 to 06cd05b69319a2c018e63db4dc30415824dc92599bf9b26487639d03fd6d4f66
[2020-06-10 15:10:25][info] Received block announce: block number 123
[2020-06-10 15:10:25][info] Received block header. Number: 123, Hash: e809b2fc697a89ac63ca18308a76bca383f946d014c3c920c689876b554ba8b4
[2020-06-10 15:10:25][info] Requesting blocks from a445da57787222aaaed37bfdca0f64981443f9554865a43abb39c5c868ff8c96 to e809b2fc697a89ac63ca18308a76bca383f946d014c3c920c689876b554ba8b4
[2020-06-10 15:10:25][info] Received blocks from: a445da57787222aaaed37bfdca0f64981443f9554865a43abb39c5c868ff8c96, to e809b2fc697a89ac63ca18308a76bca383f946d014c3c920c689876b554ba8b4
[2020-06-10 15:10:25][info] Applying block number: 1, hash: 28d20a1144d0183276c4577d478311880eff7298eecea5aeb2175408084901f5
[2020-06-10 15:10:25][warning] Resetting state to: ae7c1e51060fb97f38ff262e2620d926f69569ce020510d971115bdaf3b62087
[2020-06-10 15:10:26][info] Added block. Number: 1. Hash: 28d20a1144d0183276c4577d478311880eff7298eecea5aeb2175408084901f5. State root: feae7d10521e2737c82b56307cec9c61dfc7804af7980f1d68adc144eb85b3bc
[2020-06-10 15:10:26][info] Imported block with number: 1, hash: 28d20a1144d0183276c4577d478311880eff7298eecea5aeb2175408084901f5
[2020-06-10 15:10:26][info] Applying block number: 2, hash: 20d7bc296eb54d22fd35bab15228e4fe63a16a98a984a9f4d245b7277259c56e
[2020-06-10 15:10:26][warning] Resetting state to: feae7d10521e2737c82b56307cec9c61dfc7804af7980f1d68adc144eb85b3bc
[2020-06-10 15:10:26][info] Added block. Number: 2. Hash: 20d7bc296eb54d22fd35bab15228e4fe63a16a98a984a9f4d245b7277259c56e. State root: 8727457a1ec775acc004943c601c9a23a863c604026eba0e9e0e013e2df77326
```

Note that trie root is the same with validating and block producing nodes. When syncing node receives block announcement it first synchronizes missing blocks and then listens to the new blocks and finalization. 

### Send transaction

Like in previous tutorial we will send transfer from Alice to Bob to check that transaction was applied on every node.

Generate transaction:

```bash
subkey transfer -g a445da57787222aaaed37bfdca0f64981443f9554865a43abb39c5c868ff8c96 0xe5be9a5092b81bca64be81d212e7f2f9eba183bb7a90954f7b76361f6edb5c0a 0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48 1000 0
# Using a genesis hash of a445da57787222aaaed37bfdca0f64981443f9554865a43abb39c5c868ff8c96
# 0x2d0284ffd43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d01465669ff1d1ce7b528efcab68977a8cf1a6046b2b40786991c286f00ecab795ee72073e172ca527174f40d796a30c3824c95310d082875b44f9dde9db079ef8f0000000600ff8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48a10f
```

We can send transaction on any of the node, as it will be propagated to the block producing nodes and stored in their transaction pools until transactions is included to the block:

```bash
cd examples/transfer
python3 transfer.py localhost:11233 0x2d0284ffd43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d0168de6cd26417949dd9ac0e80b7d52aa3ad333e96f2784db75fcdbd5af904925bc80dfd3f92bc7d905e6ee7b0c9b4a59f98b228c37c7638a8c769bd7bba801a800000000600ff8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48a10f
```
