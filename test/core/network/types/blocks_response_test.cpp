/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/types/blocks_response.hpp"

#include <gmock/gmock.h>

#include <iostream>

#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/primitives/mp_utils.hpp"

using kagome::network::BlocksResponse;
using kagome::primitives::BlockBody;
using kagome::primitives::BlockData;
using kagome::primitives::BlockHeader;
using kagome::primitives::Digest;
using kagome::primitives::Justification;
using kagome::primitives::PreRuntime;

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
        Digest{PreRuntime{}}       // digest list
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

struct BlocksResponseTest : public ::testing::Test {
  BlocksResponse block_responce{1u, {createBlockData(), createBlockData()}};
};

/**
 * @given sample `block response` instance @and encoded value buffer
 * @when scale-encode `block response` instance and decode back
 * @then decoded block_response matches encoded block response
 */
TEST_F(BlocksResponseTest, EncodeSuccess) {
  EXPECT_OUTCOME_TRUE(buffer, encode(block_responce));
  EXPECT_OUTCOME_TRUE(decoded_block_response, decode<BlocksResponse>(buffer));
  ASSERT_EQ(block_responce, decoded_block_response);
}
