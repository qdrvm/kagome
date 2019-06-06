/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <gtest/gtest.h>
#include "crypto/blake2/blake2s.h"
#include "storage/merkle/polkadot_trie_db/polkadot_codec.hpp"

using namespace kagome;
using namespace common;
using namespace storage;
using namespace merkle;
using namespace testing;

struct Hash256Test
    : public ::testing::TestWithParam<std::pair<Buffer, Buffer>> {};

TEST_P(Hash256Test, Valid) {
  auto [in, out] = GetParam();
  auto codec = std::make_unique<PolkadotCodec>();
  auto actualOut = codec->hash256(in);
  EXPECT_EQ(actualOut.toHex(), out.toHex());
}

Buffer getBlake2s(const Buffer &in) {
  Buffer out(32, 0);
  blake2s(out.data(), 32, nullptr, 0, in.data(), in.size());
  return out;
}

const std::vector<std::pair<Buffer, Buffer>> cases = {
    // pair{in, out}
    {{0}, Buffer(32, 0)},  // if length < 32, do not hash, pad to 32 and return
    {{1, 3, 3, 7},
     {
         1, 3, 3, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     }},
    // buffer of size 32, consists of ones
    {Buffer(32, 1), getBlake2s(Buffer(32, 1))}};

INSTANTIATE_TEST_CASE_P(PolkadotCodec, Hash256Test, ValuesIn(cases));
