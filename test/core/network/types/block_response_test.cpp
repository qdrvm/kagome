/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/types/block_response.hpp"

#include <iostream>

#include <gmock/gmock.h>
#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/primitives/hash_creator.hpp"

using kagome::network::BlockData;
using kagome::network::BlocksResponse;
using kagome::primitives::BlockBody;
using kagome::primitives::BlockHeader;
using kagome::primitives::Justification;

using testutil::createHash256;

using kagome::scale::decode;
using kagome::scale::encode;

namespace {
  BlockHeader createBlockHeader() {
    return BlockHeader{
        createHash256({1, 1, 1}),  // parent_hash
        2u,                        // block number
        createHash256({3, 3, 3}),  // state_root
        createHash256({4, 4, 4}),  // extrinsic root
        {{5, 6, 7}}                // digest list
    };
  }

  BlockBody createBlockBody() {
    return {{{{1, 2, 3}}, {{4, 5, 6}}, {{7, 8, 9}}}};
  }

  BlockData createBlockData() {
    return BlockData{createHash256({1, 2, 3}),
                     createBlockHeader(),
                     createBlockBody(),
                     "112233"_hex2buf,
                     "445566"_hex2buf,
                     Justification{"778899"_hex2buf}};
  }

}  // namespace

struct BlockResponseTest : public ::testing::Test {
  BlocksResponse block_responce{1u, {createBlockData(), createBlockData()}};
  std::vector<uint8_t> encoded_value =
      "0100000000000000080102030000000000000000000000000000000000000000"
      "0000000000000000000101010100000000000000000000000000000000000000"
      "0000000000000000000002000000000000000303030000000000000000000000"
      "0000000000000000000000000000000000000404040000000000000000000000"
      "000000000000000000000000000000000000040C050607010C0C0102030C0405"
      "060C070809010C112233010C445566010C778899010203000000000000000000"
      "0000000000000000000000000000000000000000010101010000000000000000"
      "0000000000000000000000000000000000000000000200000000000000030303"
      "0000000000000000000000000000000000000000000000000000000000040404"
      "0000000000000000000000000000000000000000000000000000000000040C05"
      "0607010C0C0102030C0405060C070809010C112233010C445566010C778899"_unhex;
};

/**
 * @given sample `block response` instance @and encoded value buffer
 * @when scale-encode `block response` instance
 * @then result of encoding matches predefined buffer
 */
TEST_F(BlockResponseTest, EncodeSuccess) {
  EXPECT_OUTCOME_TRUE(buffer, encode(block_responce));
  ASSERT_EQ(buffer, encoded_value);
}

/**
 * @given buffer containing encoded `block response` instance
 * @and predefined `block response` instance
 * @when scale-decode that buffer
 * @then result of decoding matches predefined `block response` instance
 */
TEST_F(BlockResponseTest, DecodeSuccess) {
  EXPECT_OUTCOME_TRUE(br, decode<BlocksResponse>(encoded_value));
  ASSERT_EQ(br, block_responce);
}
