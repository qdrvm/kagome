#!/bin/bash

BUILD_DIR=./cmake-build-debug
NODE_BIN_DIR=./$BUILD_DIR/node/kagome_full/
NODE_SRC_DIR=./node/

echo "AAAA $NODE_BIN_DIR"

RUN_SECOND_NODE=$1

rm -rf $BUILD_DIR/ldb1

$NODE_BIN_DIR/kagome_full \
    --genesis $NODE_SRC_DIR/config/polkadot-v06-2nodes.json \
    --keystore $NODE_SRC_DIR/config/keystore.json \
    --leveldb $BUILD_DIR/ldb1 \
    --p2p_port 30363 \
    --rpc_http_port 40363 \
    -v 2 \
    --genesis_epoch &

if [[ $RUN_SECOND_NODE == "ON" ]]; then
  echo 'Run second node'
$NODE_BIN_DIR/kagome_full \
    --genesis $NODE_SRC_DIR/config/polkadot-v06-2nodes.json \
    --keystore $NODE_SRC_DIR/config/keystore2.json \
    --leveldb $BUILD_DIR/ldb1 \
    --p2p_port 40010 \
    --rpc_http_port 40366 \
    -v 2
fi
