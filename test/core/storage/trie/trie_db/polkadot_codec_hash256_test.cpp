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
#include "crypto/blake2/blake2b.h"
#include "storage/trie/impl/polkadot_codec.hpp"

using namespace kagome;
using namespace common;
using namespace storage;
using namespace trie;
using namespace testing;

struct Hash256Test
    : public ::testing::TestWithParam<std::pair<Buffer, Buffer>> {};

TEST_P(Hash256Test, Valid) {
  auto [in, out] = GetParam();
  auto codec = std::make_unique<PolkadotCodec>();
  auto actualOut = codec->merkleValue(in);
  EXPECT_EQ(actualOut.toHex(), out.toHex());
}

Buffer getBlake2b(const Buffer &in) {
  Buffer out(32, 0);
  blake2b(out.data(), 32, nullptr, 0, in.data(), in.size());
  return out;
}

const std::vector<std::pair<Buffer, Buffer>> cases = {
    // pair{in, out}
    {{0}, {0}},  // if length < 32, do not hash, pad to 32 and return
    {{1, 3, 3, 7},
     {
         1, 3, 3, 7}},
    // buffer of size 32, consists of ones
    {Buffer(32, 1), getBlake2b(Buffer(32, 1))}};

INSTANTIATE_TEST_CASE_P(PolkadotCodec, Hash256Test, ValuesIn(cases));
