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
#include "testutil/primitives/mp_utils.hpp"

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::network::BlockAnnounce;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockNumber;
using kagome::primitives::Digest;
using kagome::primitives::PreRuntime;

using testutil::createHash256;

using scale::decode;
using scale::encode;

struct BlockAnnounceTest : public ::testing::Test {
  void SetUp() {
    auto h1 = createHash256({1, 1, 1});
    auto h2 = createHash256({3, 3, 3});
    auto h3 = createHash256({4, 4, 4});

    block_header = BlockHeader{
        h1,                   // parent_hash
        2u,                   // block number
        h2,                   // state_root
        h3,                   // extrinsic root
        Digest{{PreRuntime{}}}  // digest list
    };

    block_announce = BlockAnnounce{block_header};
  }

  BlockHeader block_header;

  /// `block announce` instance
  BlockAnnounce block_announce;
};

/**
 * @given sample `block announce` instance @and encoded value buffer
 * @when scale-encode `block announce` instance and decode back
 * @then decoded block announce matches initial one
 */
TEST_F(BlockAnnounceTest, EncodeSuccess) {
  EXPECT_OUTCOME_TRUE(buffer, encode(block_announce));
  EXPECT_OUTCOME_TRUE(ba, decode<BlockAnnounce>(buffer));
  ASSERT_EQ(block_announce, ba);
}
