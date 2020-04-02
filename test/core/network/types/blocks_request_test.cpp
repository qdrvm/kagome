/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/types/blocks_request.hpp"
#include <gmock/gmock.h>
#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/primitives/mp_utils.hpp"
#include "testutil/testparam.hpp"

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::network::BlockAttributes;
using kagome::network::BlockAttributesBits;
using kagome::network::BlocksRequest;
using kagome::network::Direction;

using kagome::scale::decode;
using kagome::scale::encode;

/**
 * @given sample `block request` instance @and encoded value buffer
 * @when scale-encode `block request` instance
 * @then result of encoding matches predefined buffer
 */
TEST(BlocksRequestTest, EncodeSuccess) {
  using Bits = BlockAttributesBits;
  Hash256 hash;
  hash.fill(2);
  BlocksRequest block_request{1,
                              {Bits::BODY | Bits::HEADER | Bits::RECEIPT},
                              2u,
                              hash,
                              Direction::DESCENDING,
                              {5u}};

  EXPECT_OUTCOME_TRUE(buffer, encode(block_request));
  EXPECT_OUTCOME_TRUE(decoded, decode<BlocksRequest>(buffer));
  ASSERT_EQ(block_request, decoded);
}
