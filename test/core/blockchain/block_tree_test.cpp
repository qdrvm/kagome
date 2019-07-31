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
#include "primitives/justification.hpp"
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

  /**
   * Add a block with some data, which is a child of the top-most block
   * @return block, which was added, along with its hash
   */
  BlockHash addBlock(const Block &block) {
    EXPECT_CALL(db_, put(_, _))
        .Times(4)
        .WillRepeatedly(Return(outcome::success()));
    EXPECT_CALL(db_, put(_, Buffer{scale::encode(block.header).value()}))
        .WillOnce(Return(outcome::success()));
    EXPECT_CALL(db_, put(_, Buffer{scale::encode(block.body).value()}))
        .WillOnce(Return(outcome::success()));

    EXPECT_TRUE(block_tree_->addBlock(block));

    auto encoded_block = scale::encode(block).value();
    return hasher_->blake2_256(encoded_block);
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
  std::shared_ptr<crypto::Hasher> hasher_ =
      std::make_shared<crypto::HasherImpl>();

  std::unique_ptr<LevelDbBlockTree> block_tree_;

  BlockHeader finalized_block_header_{.number = 0,
                                      .digest = Buffer{0x11, 0x33}};
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
  // GIVEN

  // WHEN
  EXPECT_CALL(db_, get(_))
      .WillOnce(Return(kFinalizedBlockLookupKey))
      .WillOnce(Return(Buffer{encoded_finalized_block_body_}));

  // THEN
  EXPECT_OUTCOME_TRUE(body, block_tree_->getBlockBody(kLastFinalizedBlockId))
  ASSERT_EQ(body, finalized_block_body_);
}

/**
 * @given block tree with at least one block inside
 * @when adding a new block, which is a child of that block
 * @then block is added
 */
TEST_F(BlockTreeTest, AddBlock) {
  // GIVEN
  auto &&deepest_block_hash = block_tree_->deepestLeaf();
  ASSERT_EQ(deepest_block_hash, kFinalizedBlockHash);

  auto leaves = block_tree_->getLeaves();
  ASSERT_EQ(leaves.size(), 1);
  ASSERT_EQ(leaves[0], kFinalizedBlockHash);

  auto children_res = block_tree_->getChildren(kFinalizedBlockHash);
  ASSERT_TRUE(children_res);
  ASSERT_TRUE(children_res.value().empty());

  // WHEN
  BlockHeader header{.parent_hash = kFinalizedBlockHash,
                     .number = 1,
                     .digest = Buffer{0x66, 0x44}};
  BlockBody body{{Buffer{0x55, 0x55}}};
  Block new_block{header, body};
  auto hash = addBlock(new_block);

  // THEN
  auto &&new_deepest_block_hash = block_tree_->deepestLeaf();
  ASSERT_EQ(new_deepest_block_hash, hash);

  leaves = block_tree_->getLeaves();
  ASSERT_EQ(leaves.size(), 1);
  ASSERT_EQ(leaves[0], hash);

  children_res = block_tree_->getChildren(hash);
  ASSERT_TRUE(children_res);
  ASSERT_TRUE(children_res.value().empty());
}

/**
 * @given block tree with at least one block inside
 * @when adding a new block, which is not a child of any block inside
 * @then corresponding error is returned
 */
TEST_F(BlockTreeTest, AddBlockNoParent) {
  // GIVEN
  BlockHeader header{.digest = Buffer{0x66, 0x44}};
  BlockBody body{{Buffer{0x55, 0x55}}};
  Block new_block{header, body};

  // WHEN
  EXPECT_OUTCOME_FALSE(err, block_tree_->addBlock(new_block));

  // THEN
  ASSERT_EQ(err, LevelDbBlockTree::Error::NO_PARENT);
}

/**
 * @given block tree with at least two blocks inside
 * @when finalizing a non-finalized block
 * @then finalization completes successfully
 */
TEST_F(BlockTreeTest, Finalize) {
  // GIVEN
  auto &&last_finalized_hash = block_tree_->getLastFinalized();
  ASSERT_EQ(last_finalized_hash, kFinalizedBlockHash);

  BlockHeader header{.parent_hash = kFinalizedBlockHash,
                     .number = 1,
                     .digest = Buffer{0x66, 0x44}};
  BlockBody body{{Buffer{0x55, 0x55}}};
  Block new_block{header, body};
  auto hash = addBlock(new_block);

  Justification justification{{0x45, 0xF4}};
  auto encoded_justification = scale::encode(justification).value();
  EXPECT_CALL(db_, put(_, _))
      .Times(2)
      .WillRepeatedly(Return(outcome::success()));
  EXPECT_CALL(db_, put(_, Buffer{encoded_justification}))
      .WillOnce(Return(outcome::success()));

  // WHEN
  ASSERT_TRUE(block_tree_->finalize(hash, justification));

  // THEN
  ASSERT_EQ(block_tree_->getLastFinalized(), hash);
}

/**
 * @given block tree with at least three blocks inside
 * @when asking for chain from the lowest block
 * @then chain from that block to the last finalized one is returned
 */
TEST_F(BlockTreeTest, GetChainByBlock) {
  // GIVEN
  BlockHeader header{.parent_hash = kFinalizedBlockHash,
                     .number = 1,
                     .digest = Buffer{0x66, 0x44}};
  BlockBody body{{Buffer{0x55, 0x55}}};
  Block new_block{header, body};
  auto hash1 = addBlock(new_block);

  header = BlockHeader{
      .parent_hash = hash1, .number = 2, .digest = Buffer{0x66, 0x55}};
  body = BlockBody{{Buffer{0x55, 0x55}}};
  new_block = Block{header, body};
  auto hash2 = addBlock(new_block);

  std::vector<BlockHash> expected_chain{hash2, hash1, kFinalizedBlockHash};

  // WHEN
  EXPECT_OUTCOME_TRUE(chain, block_tree_->getChainByBlock(hash2))

  // THEN
  ASSERT_EQ(chain, expected_chain);
}
