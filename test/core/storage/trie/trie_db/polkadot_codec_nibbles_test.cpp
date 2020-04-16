/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <memory>

#include <gtest/gtest.h>
#include "storage/trie/impl/polkadot_codec.hpp"

using namespace kagome;
using namespace common;
using namespace storage;
using namespace trie;

struct NibblesToKey
  // pair{nibbles, key}
    : public ::testing::TestWithParam<std::pair<Buffer, Buffer>> {};

struct KeyToNibbles
  // pair{nibbles, key}
    : public ::testing::TestWithParam<std::pair<Buffer, Buffer>> {};

TEST_P(NibblesToKey, nibblesToKey) {
  auto codec = std::make_unique<PolkadotCodec>();
  auto [nibbles, key] = GetParam();
  auto actualKey = codec->nibblesToKey(nibbles);
  ASSERT_EQ(key, actualKey);
}

TEST_P(KeyToNibbles, keyToNibbles) {
  auto codec = std::make_unique<PolkadotCodec>();
  auto [nibbles, key] = GetParam();
  auto actualNibbles = codec->keyToNibbles(key);
  ASSERT_EQ(nibbles, actualNibbles);
}

const std::vector<std::pair<Buffer, Buffer>> KEY_TO_NIBBLES = {
    {{0, 0}, {0x0}},
    {{0xF, 0xF}, {0xFF}},
    {{0x3, 0xa, 0x0, 0x5}, {0x3a, 0x05}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1}, {0xAA, 0xFF, 0x01}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1, 0xc, 0x2}, {0xAA, 0xFF, 0x01, 0xc2}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1, 0xc, 0x0}, {0xAA, 0xFF, 0x01, 0xc0}},
};

const std::vector<std::pair<Buffer, Buffer>> NIBBLES_TO_KEY_LE = {
    {{0xF, 0xF}, {0xFF}},
    {{0x3, 0xa, 0x0, 0x5}, {0x3a, 0x05}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1}, {0xaa, 0xff, 0x01}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1, 0xc, 0x2}, {0xaa, 0xff, 0x01, 0xc2}},
    {{0xa, 0xa, 0xf, 0xf, 0x0, 0x1, 0xc}, {0xa, 0xaf, 0xf0, 0x1c}}
};

INSTANTIATE_TEST_CASE_P(KeyToNibbles, KeyToNibbles, ::testing::ValuesIn(KEY_TO_NIBBLES));
INSTANTIATE_TEST_CASE_P(NibblesToKeyLE, NibblesToKey, ::testing::ValuesIn(NIBBLES_TO_KEY_LE));
