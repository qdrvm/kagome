/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <gtest/gtest.h>
#include "storage/trie/polkadot_trie_db/polkadot_codec.hpp"

using namespace kagome;
using namespace common;
using namespace storage;
using namespace trie;

struct EvenNibbles
    // pair{nibbles, key}
    : public ::testing::TestWithParam<std::pair<Buffer, Buffer>> {};

struct OddNibbles
    // pair{nibbles, key}
    : public ::testing::TestWithParam<std::pair<Buffer, Buffer>> {};

TEST_P(EvenNibbles, KeyToNibbles) {
  auto codec = std::make_unique<PolkadotCodec>();
  auto [nibbles, key] = GetParam();
  auto actualNibbles = codec->keyToNibbles(key);
  ASSERT_EQ(actualNibbles, nibbles);
}

TEST_P(EvenNibbles, NibblesToKey) {
  auto codec = std::make_unique<PolkadotCodec>();
  auto [nibbles, key] = GetParam();
  auto actualKey = codec->nibblesToKey(nibbles);
  ASSERT_EQ(key, actualKey);
}

TEST_P(OddNibbles, NibblesToKey) {
  auto codec = std::make_unique<PolkadotCodec>();
  auto [nibbles, key] = GetParam();
  auto actualKey = codec->nibblesToKey(nibbles);
  ASSERT_EQ(key, actualKey);
}

// those cases contain even number of nibbles && last nibble != 0
const std::vector<std::pair<Buffer, Buffer>> EVEN = {
    // pair{nibbles, key}
    {{}, {}},
    {{0, 0, 2, 1, 4, 3, 0, 5, 5, 0, 0xf, 0xf},
     {0x00, 0x12, 0x34, 0x50, 0x05, 0xff}},
    {{5, 5}, {0x55}},
    {{5, 5, 5, 5}, {0x55, 0x55}},
    {{0, 1}, {0x10}},
};

// those cases contain either even number of nibbles && last nibble == 0
// or odd number of nibbles
const std::vector<std::pair<Buffer, Buffer>> ODD = {
    // pair{nibbles, key}
    {{0, 0}, {0x00}},
    {{1, 0}, {0x01}},
    {{0, 0, 0, 0}, {0x00, 0x00}},
    {{0}, {0x00}},
    {{0, 0, 0}, {0x00, 0x00}},
    {{0, 0, 0, 0, 0}, {0x00, 0x00, 0x00}},
    {{5}, {0x05}},
    {{0, 0, 5}, {0x00, 0x05}},
    {{0, 0, 0, 0, 5}, {0x00, 0x00, 0x05}},
    {{1}, {0x01}},
    {{1, 1, 1}, {0x11, 0x01}},
    {{1, 1, 1, 1, 1}, {0x11, 0x11, 0x01}},
};

INSTANTIATE_TEST_CASE_P(PolkadotCodec, EvenNibbles, ::testing::ValuesIn(EVEN));

INSTANTIATE_TEST_CASE_P(PolkadotCodec, OddNibbles, ::testing::ValuesIn(ODD));
