/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/level_db_block_tree.hpp"

#include <gtest/gtest.h>
#include "blockchain/impl/level_db_util.hpp"
#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "mock/core/storage/persistent_map_mock.hpp"
#include "primitives/block_id.hpp"
#include "scale/scale.hpp"
#include "testutil/outcome.hpp"

using namespace kagome;
using namespace storage;
using namespace common;
using namespace primitives;
using namespace blockchain;

using prefix::Prefix;
using testing::_;
using testing::Return;

struct BlockTreeTest : public testing::Test {
  void SetUp() override {
    // for LevelDbBlockTree::create(..)
    EXPECT_CALL(db_, get(_))
        .WillOnce(Return(kFinalizedBlockLookupKey))
        .WillOnce(Return(Buffer{encoded_finalized_block_header_}));

    block_tree_ =
        LevelDbBlockTree::create(db_, kLastFinalizedBlockId, hasher_).value();
  }

  const Buffer kFinalizedBlockLookupKey{0x12, 0x85};
  const Buffer kFinalizedBlockHashWithKey =
      Buffer{}.putUint8(Prefix::ID_TO_LOOKUP_KEY).put(kFinalizedBlockHash);
  const Buffer kFinalizedBlockHashWithKeyAndHeader =
      Buffer{}.putUint8(Prefix::HEADER).putBuffer(kFinalizedBlockLookupKey);
  const Buffer kFinalizedBlockHashWithKeyAndBody =
      Buffer{}.putUint8(Prefix::BODY).putBuffer(kFinalizedBlockLookupKey);

  const BlockHash kFinalizedBlockHash =
      BlockHash::fromString("andj4kdn4odnfkslfn3k4jdnbmeodkv4").value();

  face::PersistentMapMock<Buffer, Buffer> db_;
  const BlockId kLastFinalizedBlockId = kFinalizedBlockHash;
  std::shared_ptr<hash::Hasher> hasher_ = std::make_shared<hash::HasherImpl>();

  std::unique_ptr<LevelDbBlockTree> block_tree_;

  BlockHeader finalized_block_header_{.digest = Buffer{0x11, 0x33}};
  std::vector<uint8_t> encoded_finalized_block_header_ =
      scale::encode(finalized_block_header_).value();

  BlockBody finalized_block_body_{{Buffer{0x22, 0x44}}, {Buffer{0x55, 0x66}}};
  std::vector<uint8_t> encoded_finalized_block_body_ =
      scale::encode(finalized_block_body_).value();
};

/**
 * @given block tree with at least one block inside
 * @when requesting body of that block
 * @then body is returned
 */
TEST_F(BlockTreeTest, GetBody) {
  EXPECT_CALL(db_, get(_))
      .WillOnce(Return(kFinalizedBlockLookupKey))
      .WillOnce(Return(Buffer{encoded_finalized_block_body_}));

  EXPECT_OUTCOME_TRUE(body, block_tree_->getBlockBody(kLastFinalizedBlockId))
  ASSERT_EQ(body, finalized_block_body_);
}
