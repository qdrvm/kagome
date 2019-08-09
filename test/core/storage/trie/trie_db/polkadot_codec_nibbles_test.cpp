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

// tests are taken from Go for compatibility
struct NibblesCompat
  // pair{nibbles, key}
    : public ::testing::TestWithParam<std::pair<Buffer, Buffer>> {};

struct NibblesCompatLE
  // pair{nibbles, key}
    : public ::testing::TestWithParam<std::pair<Buffer, Buffer>> {};

struct KeyToNibblesCompat
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

TEST_P(NibblesCompat, NibblesToKey) {
  auto codec = std::make_unique<PolkadotCodec>();
  auto [nibbles, key] = GetParam();
  auto actualKey = codec->nibblesToKey(nibbles);
  ASSERT_EQ(key, actualKey);
}

TEST_P(NibblesCompatLE, NibblesToKeyLE) {
  auto codec = std::make_unique<PolkadotCodec>();
  auto [nibbles, key] = GetParam();
//  auto actualKey = codec->nibblesToKeyLE(nibbles);
//  ASSERT_EQ(key, actualKey);
}

TEST_P(KeyToNibblesCompat, keyToNibbles) {
  auto codec = std::make_unique<PolkadotCodec>();
  auto [nibbles, key] = GetParam();
  auto actualNibbles = codec->keyToNibbles(key);
  ASSERT_EQ(nibbles, actualNibbles);
}

// those cases contain even number of nibbles && last nibble != 0
const std::vector<std::pair<Buffer, Buffer>> EVEN = {
    // pair{nibbles, key}
    {{}, {}},
    {{0, 0, 2, 1, 4, 3, 0, 5, 5, 0, 0xf, 0xf},
     {0x00, 0x12, 0x34, 0x50, 0x05, 0xff}},
    {{5, 5}, {0x55}},
    {{5, 5, 5, 5}, {0x55, 0x55}},
    {{0, 1}, {0x10}}
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
    {{1, 1, 1, 1, 1}, {0x11, 0x11, 0x01}}
};

const std::vector<std::pair<Buffer, Buffer>> KEY_TO_NIBBLES = {
    {{0, 0}, {0x0}},
    {{0xF, 0xF}, {0xFF}},
    {{0x3, 0xa, 0x0, 0x5}, {0x3a, 0x05}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1}, {0xAA, 0xFF, 0x01}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1, 0xc, 0x2}, {0xAA, 0xFF, 0x01, 0xc2}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1, 0xc, 0x0}, {0xAA, 0xFF, 0x01, 0xc0}},
};

const std::vector<std::pair<Buffer, Buffer>> NIBBLES_TO_KEY = {
    {{0xF, 0xF}, {0xFF}},
    {{0x3, 0xa, 0x0, 0x5}, {0xa3, 0x50}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1}, {0xaa, 0xff, 0x10}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1, 0xc, 0x2}, {0xaa, 0xff, 0x10, 0x2c}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1, 0xc}, {0xaa, 0xff, 0x10, 0x0c}}
};

const std::vector<std::pair<Buffer, Buffer>> NIBBLES_TO_KEY_LE = {
    {{0xF, 0xF}, {0xFF}},
    {{0x3, 0xa, 0x0, 0x5}, {0x3a, 0x05}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1}, {0xaa, 0xff, 0x01}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1, 0xc, 0x2}, {0xaa, 0xff, 0x01, 0xc2}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1, 0xc}, {0xa, 0xaf, 0xf0, 0x1c}}
};

INSTANTIATE_TEST_CASE_P(PolkadotCodec, EvenNibbles, ::testing::ValuesIn(EVEN));

INSTANTIATE_TEST_CASE_P(PolkadotCodec, OddNibbles, ::testing::ValuesIn(ODD));

INSTANTIATE_TEST_CASE_P(NibblesToKey, NibblesCompat, ::testing::ValuesIn(NIBBLES_TO_KEY));
INSTANTIATE_TEST_CASE_P(KeyToNibbles, KeyToNibblesCompat, ::testing::ValuesIn(KEY_TO_NIBBLES));
INSTANTIATE_TEST_CASE_P(NibblesToKeyLE, NibblesCompatLE, ::testing::ValuesIn(NIBBLES_TO_KEY_LE));
