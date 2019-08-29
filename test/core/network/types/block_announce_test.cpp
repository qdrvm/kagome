/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "network/types/block_announce.hpp"

#include <iostream>

#include <gmock/gmock.h>
#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/primitives/hash_creator.hpp"

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::network::BlockAnnounce;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockNumber;
using kagome::primitives::Digest;

using testutil::createHash256;

using kagome::scale::decode;
using kagome::scale::encode;

struct BlockAnnounceTest : public ::testing::Test {
  BlockHeader block_header{
      createHash256({1, 1, 1}),  // parent_hash
      2u,                        // block number
      createHash256({3, 3, 3}),  // state_root
      createHash256({4, 4, 4}),  // extrinsic root
      {{5, 6, 7}}                // digest list
  };

  /// `block announce` instance
  BlockAnnounce block_announce{block_header};

  /// scale-encoded `block announce` buffer
  std::vector<uint8_t> encoded_value =
      "0101010000000000000000000000000000000000000000000000000000"
      "0000000200000000000000030303000000000000000000000000000000"
      "0000000000000000000000000000040404000000000000000000000000"
      "0000000000000000000000000000000000040C050607"_unhex;
};

/**
 * @given sample `block announce` instance @and encoded value buffer
 * @when scale-encode `block announce` instance
 * @then result of encoding matches predefined buffer
 */
TEST_F(BlockAnnounceTest, EncodeSuccess) {
  EXPECT_OUTCOME_TRUE(buffer, encode(block_announce));
  ASSERT_EQ(buffer, encoded_value);
}

/**
 * @given buffer containing encoded `block announce` instance
 * @and predefined `block announce` instance
 * @when scale-decode that buffer
 * @then result of decoding matches predefined `block announce` instance
 */
TEST_F(BlockAnnounceTest, DecodeSuccess) {
  EXPECT_OUTCOME_TRUE(ba, decode<BlockAnnounce>(encoded_value));
  ASSERT_EQ(ba, block_announce);
}
