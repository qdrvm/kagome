/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/types/block_request.hpp"

#include <gmock/gmock.h>
#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/primitives/hash_creator.hpp"

using kagome::common::Buffer;
using kagome::network::BlockAttributes;
using kagome::network::BlockAttributesBits;
using kagome::network::BlockRequest;
using kagome::network::Direction;

using testutil::createHash256;

using kagome::scale::decode;
using kagome::scale::encode;

using kagome::network::operator|;

struct BlockRequestTest : public ::testing::Test {
  using Bits = BlockAttributesBits;

  BlockRequest block_request{1u,
                             {Bits::BODY | Bits::HEADER | Bits::RECEIPT},
                             2u,
                             createHash256({3, 4, 5}),
                             Direction::DESCENDING,
                             {5u}};
  std::vector<uint8_t> encoded_value =
      "010000000000000007010200000000000000"
      "010304050000000000000000000000000000"
      "00000000000000000000000000000001"
      "0105000000"_unhex;
};

/**
 * @given sample `block request` instance @and encoded value buffer
 * @when scale-encode `block request` instance
 * @then result of encoding matches predefined buffer
 */
TEST_F(BlockRequestTest, EncodeSuccess) {
  EXPECT_OUTCOME_TRUE(buffer, encode(block_request));
  ASSERT_EQ(buffer, encoded_value);
}

/**
 * @given buffer containing encoded `block request` instance
 * @and predefined `block request` instance
 * @when scale-decode that buffer
 * @then result of decoding matches predefined `block request` instance
 */
TEST_F(BlockRequestTest, DecodeSuccess) {
  EXPECT_OUTCOME_TRUE(br, decode<BlockRequest>(encoded_value));
  ASSERT_EQ(br, block_request);
}
