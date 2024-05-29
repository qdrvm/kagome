#!/bin/sh -eux
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

DIR=.zombienet-precompile
mkdir -p $DIR
echo '*' > $DIR/.gitignore

polkadot build-spec --chain rococo-local --raw > $DIR/rococo-local.json
adder-collator export-genesis-wasm > $DIR/adder-collator.json
undying-collator export-genesis-wasm > $DIR/undying-collator.json
polkadot-parachain export-genesis-wasm > $DIR/polkadot-parachain.json
polkadot build-spec --chain westend-local --raw > $DIR/westend-local.json

kagome --tmp --chain $DIR/rococo-local.json \
  --precompile-para $DIR/adder-collator.json \
  --precompile-para $DIR/undying-collator.json \
  --precompile-para $DIR/polkadot-parachain.json

kagome --tmp --chain $DIR/westend-local.json \
  --precompile-para $DIR/polkadot-parachain.json

kagome --tmp --chain ./0009-basic-warp-sync/gen-db-raw.json \
  --precompile-relay
