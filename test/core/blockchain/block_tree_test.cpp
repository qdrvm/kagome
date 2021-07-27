/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "blockchain/impl/block_tree_impl.hpp"

#include "blockchain/block_tree_error.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "common/blob.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "mock/core/api/service/author/author_api_mock.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/blockchain/block_storage_mock.hpp"
#include "mock/core/clock/clock_mock.hpp"
#include "mock/core/consensus/babe/babe_util_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/storage/persistent_map_mock.hpp"
#include "network/impl/extrinsic_observer_impl.hpp"
#include "primitives/block_id.hpp"
#include "primitives/justification.hpp"
#include "scale/scale.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using namespace storage;
using namespace common;
using namespace clock;
using namespace consensus;
using namespace primitives;
using namespace blockchain;

using namespace std::chrono_literals;

using prefix::Prefix;
using testing::_;
using testing::Return;

struct BlockTreeTest : public testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    // for LevelDbBlockTree::create(..)
    EXPECT_CALL(*storage_, getBlockHeader(kLastFinalizedBlockId))
        .WillOnce(Return(finalized_block_header_));

    EXPECT_CALL(*header_repo_, getNumberByHash(kFinalizedBlockInfo.hash))
        .WillRepeatedly(Return(kFinalizedBlockInfo.number));

    EXPECT_CALL(*header_repo_, getHashByNumber(kFinalizedBlockInfo.number))
        .WillRepeatedly(Return(kFinalizedBlockInfo.hash));

    auto chain_events_engine =
        std::make_shared<primitives::events::ChainSubscriptionEngine>();
    auto ext_events_engine =
        std::make_shared<primitives::events::ExtrinsicSubscriptionEngine>();

    auto extrinsic_event_key_repo =
        std::make_shared<subscription::ExtrinsicEventKeyRepository>();

    clock_ = std::make_shared<SystemClockMock>();

    babe_config_ = std::make_shared<primitives::BabeConfiguration>();
    babe_config_->slot_duration = 60ms;
    babe_config_->randomness.fill(0);
    babe_config_->genesis_authorities = {primitives::Authority{{}, 1}};
    babe_config_->leadership_rate = {1, 4};
    babe_config_->epoch_length = 2;

    babe_util_ = std::make_shared<BabeUtilMock>();
    EXPECT_CALL(*babe_util_, slotToEpoch(_)).WillRepeatedly(Return(0));

    block_tree_ = BlockTreeImpl::create(header_repo_,
                                        storage_,
                                        kLastFinalizedBlockId,
                                        extrinsic_observer_,
                                        hasher_,
                                        chain_events_engine,
                                        ext_events_engine,
                                        extrinsic_event_key_repo,
                                        runtime_core_,
                                        babe_config_,
                                        babe_util_)
                      .value();
  }

  /**
   * Add a block with some data, which is a child of the top-most block
   * @return block, which was added, along with its hash
   */
  BlockHash addBlock(const Block &block) {
    auto encoded_block = scale::encode(block).value();
    auto hash = hasher_->blake2b_256(encoded_block);

    EXPECT_CALL(*storage_, putBlock(block)).WillRepeatedly(Return(hash));
    EXPECT_TRUE(block_tree_->addBlock(block));

    EXPECT_CALL(*header_repo_, getBlockHeader(primitives::BlockId(hash)))
        .WillRepeatedly(Return(block.header));
    EXPECT_CALL(*header_repo_, getHashByNumber(block.header.number))
        .WillRepeatedly(Return(hash));

    return hash;
  }

  BlockHash addHeaderToRepository(const BlockId &parent, BlockNumber number) {
    BlockHeader header;
    header.parent_hash = boost::get<BlockHash>(parent);
    header.number = number;

    return addBlock(Block{header, {}});
  }

  const BlockInfo kFinalizedBlockInfo{
      42ul, BlockHash::fromString("andj4kdn4odnfkslfn3k4jdnbmeodkv4").value()};

  std::shared_ptr<BlockHeaderRepositoryMock> header_repo_ =
      std::make_shared<BlockHeaderRepositoryMock>();

  std::shared_ptr<BlockStorageMock> storage_ =
      std::make_shared<BlockStorageMock>();

  std::shared_ptr<network::ExtrinsicObserver> extrinsic_observer_ =
      std::make_shared<network::ExtrinsicObserverImpl>(
          std::make_shared<api::AuthorApiMock>());

  std::shared_ptr<crypto::Hasher> hasher_ =
      std::make_shared<crypto::HasherImpl>();

  std::shared_ptr<runtime::CoreMock> runtime_core_ =
      std::make_shared<runtime::CoreMock>();

  std::shared_ptr<SystemClockMock> clock_;
  std::shared_ptr<primitives::BabeConfiguration> babe_config_;
  std::shared_ptr<BabeUtilMock> babe_util_;

  std::shared_ptr<BlockTreeImpl> block_tree_;

  const BlockId kLastFinalizedBlockId = kFinalizedBlockInfo.hash;

  BlockHeader finalized_block_header_{.number = kFinalizedBlockInfo.number,
                                      .digest = {{PreRuntime{}}}};

  BlockBody finalized_block_body_{{Buffer{0x22, 0x44}}, {Buffer{0x55, 0x66}}};
};

/**
 * @given block tree with at least one block inside
 * @when requesting body of that block
 * @then body is returned
 */
TEST_F(BlockTreeTest, GetBody) {
  // GIVEN
  // WHEN
  EXPECT_CALL(*storage_, getBlockBody(_))
      .WillOnce(Return(finalized_block_body_));

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
  auto &&[_, deepest_block_hash] = block_tree_->deepestLeaf();
  ASSERT_EQ(deepest_block_hash, kFinalizedBlockInfo.hash);

  auto leaves = block_tree_->getLeaves();
  ASSERT_EQ(leaves.size(), 1);
  ASSERT_EQ(leaves[0], kFinalizedBlockInfo.hash);

  auto children_res = block_tree_->getChildren(kFinalizedBlockInfo.hash);
  ASSERT_TRUE(children_res);
  ASSERT_TRUE(children_res.value().empty());

  // WHEN
  BlockHeader header{.parent_hash = kFinalizedBlockInfo.hash,
                     .number = kFinalizedBlockInfo.number + 1,
                     .digest = {PreRuntime{}}};
  BlockBody body{{Buffer{0x55, 0x55}}};
  Block new_block{header, body};
  auto hash = addBlock(new_block);

  // THEN
  auto &&[__, new_deepest_block_hash] = block_tree_->deepestLeaf();
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
  BlockHeader header{.digest = {PreRuntime{}}};
  BlockBody body{{Buffer{0x55, 0x55}}};
  Block new_block{header, body};

  // WHEN
  EXPECT_OUTCOME_FALSE(err, block_tree_->addBlock(new_block));

  // THEN
  ASSERT_EQ(err, BlockTreeError::NO_PARENT);
}

/**
 * @given block tree with at least two blocks inside
 * @when finalizing a non-finalized block
 * @then finalization completes successfully
 */
TEST_F(BlockTreeTest, Finalize) {
  // GIVEN
  auto &&last_finalized_hash = block_tree_->getLastFinalized().hash;
  ASSERT_EQ(last_finalized_hash, kFinalizedBlockInfo.hash);

  BlockHeader header{.parent_hash = kFinalizedBlockInfo.hash,
                     .number = kFinalizedBlockInfo.number + 1,
                     .digest = {PreRuntime{}}};
  BlockBody body{{Buffer{0x55, 0x55}}};
  Block new_block{header, body};
  auto hash = addBlock(new_block);

  primitives::BlockId bid = hash;

  Justification justification{{0x45, 0xF4}};
  auto encoded_justification = scale::encode(justification).value();
  EXPECT_CALL(*storage_, getJustification(primitives::BlockId(hash)))
      .WillOnce(Return(outcome::failure(boost::system::error_code{})));
  EXPECT_CALL(*storage_, putJustification(justification, hash, header.number))
      .WillRepeatedly(Return(outcome::success()));
  EXPECT_CALL(*storage_, setLastFinalizedBlockHash(hash))
      .WillRepeatedly(Return(outcome::success()));
  EXPECT_CALL(*storage_, getBlockHeader(bid))
      .WillRepeatedly(Return(outcome::success(header)));
  EXPECT_CALL(*storage_, getBlockBody(bid))
      .WillRepeatedly(Return(outcome::success(body)));
  EXPECT_CALL(*runtime_core_, version())
      .WillRepeatedly(Return(primitives::Version{}));

  // WHEN
  ASSERT_TRUE(block_tree_->finalize(hash, justification));

  // THEN
  ASSERT_EQ(block_tree_->getLastFinalized().hash, hash);
}

/**
 * @given block tree with at least three blocks inside
 * @when asking for chain from the lowest block to the closest finalized one
 * @then chain from that block to the last finalized one is returned
 */
TEST_F(BlockTreeTest, GetChainByBlockOnly) {
  // GIVEN
  BlockHeader header1{.parent_hash = kFinalizedBlockInfo.hash,
                      .number = kFinalizedBlockInfo.number + 1,
                      .digest = {PreRuntime{}}};
  BlockBody body1{{Buffer{0x55, 0x55}}};
  Block block1{header1, body1};
  auto hash1 = addBlock(block1);

  BlockHeader header2{.parent_hash = hash1,
                      .number = header1.number + 1,
                      .digest = {Consensus{}}};
  BlockBody body2{{Buffer{0x55, 0x55}}};
  Block block2{header2, body2};
  auto hash2 = addBlock(block2);

  std::vector<BlockHash> expected_chain{kFinalizedBlockInfo.hash, hash1, hash2};

  // WHEN
  EXPECT_OUTCOME_TRUE(chain, block_tree_->getChainByBlock(hash2))

  // THEN
  ASSERT_EQ(chain, expected_chain);
}

/**
 * @given block tree with at least three blocks inside
 * @when asking for chain from the given block to top
 * @then expected chain is returned
 */
TEST_F(BlockTreeTest, GetChainByBlockAscending) {
  // GIVEN
  BlockHeader header{.parent_hash = kFinalizedBlockInfo.hash,
                     .number = kFinalizedBlockInfo.number + 1,
                     .digest = {PreRuntime{}}};
  BlockBody body{{Buffer{0x55, 0x55}}};
  Block new_block{header, body};
  auto hash1 = addBlock(new_block);

  header = BlockHeader{.parent_hash = hash1,
                       .number = kFinalizedBlockInfo.number + 2,
                       .digest = {Consensus{}}};
  body = BlockBody{{Buffer{0x55, 0x55}}};
  new_block = Block{header, body};
  auto hash2 = addBlock(new_block);

  std::vector<BlockHash> expected_chain{kFinalizedBlockInfo.hash, hash1, hash2};

  // WHEN
  EXPECT_OUTCOME_TRUE(
      chain,
      block_tree_->getChainByBlock(
          kFinalizedBlockInfo.hash, BlockTree::GetChainDirection::ASCEND, 5));

  // THEN
  ASSERT_EQ(chain, expected_chain);
}

/**
 * @given block tree with at least three blocks inside
 * @when asking for chain from the given block to bottom
 * @then expected chain is returned
 */
TEST_F(BlockTreeTest, GetChainByBlockDescending) {
  // GIVEN
  BlockHeader header{.parent_hash = kFinalizedBlockInfo.hash,
                     .number = kFinalizedBlockInfo.number + 1,
                     .digest = {PreRuntime{}}};
  BlockBody body{{Buffer{0x55, 0x55}}};
  Block new_block{header, body};
  auto hash1 = addBlock(new_block);

  header = BlockHeader{.parent_hash = hash1,
                       .number = kFinalizedBlockInfo.number + 2,
                       .digest = {Consensus{}}};
  body = BlockBody{{Buffer{0x55, 0x55}}};
  new_block = Block{header, body};
  auto hash2 = addBlock(new_block);

  EXPECT_CALL(*header_repo_, getNumberByHash(kFinalizedBlockInfo.hash))
      .WillRepeatedly(Return(0));
  EXPECT_CALL(*header_repo_, getNumberByHash(hash2)).WillRepeatedly(Return(2));
  EXPECT_CALL(*header_repo_, getHashByNumber(0))
      .WillOnce(Return(kFinalizedBlockInfo.hash));

  std::vector<BlockHash> expected_chain{hash2, hash1, kFinalizedBlockInfo.hash};

  // WHEN
  EXPECT_OUTCOME_TRUE(chain,
                      block_tree_->getChainByBlock(
                          hash2, BlockTree::GetChainDirection::DESCEND, 5));

  // THEN
  ASSERT_EQ(chain, expected_chain);
}

/**
 * @given a block tree with one block in it
 * @when trying to obtain the best chain that contais a block, which is
 * present in the storage, but is not connected to the base block in the tree
 * @then BLOCK_NOT_FOUND error is returned
 */
TEST_F(BlockTreeTest, GetBestChain_BlockNotFound) {
  EXPECT_CALL(*header_repo_, getBlockHeader(kLastFinalizedBlockId))
      .WillRepeatedly(Return(finalized_block_header_));

  BlockHash target_hash({1, 1, 1});
  BlockHeader target_header;
  target_header.number = 1337;
  EXPECT_CALL(*header_repo_, getBlockHeader(primitives::BlockId(target_hash)))
      .WillRepeatedly(Return(target_header));
  EXPECT_CALL(*header_repo_, getHashByNumber(target_header.number))
      .WillRepeatedly(Return(target_hash));

  EXPECT_OUTCOME_FALSE(
      best_info, block_tree_->getBestContaining(target_hash, boost::none));
  ASSERT_EQ(best_info, BlockTreeImpl::Error::BLOCK_NOT_FOUND);
}

/**
 * @given a block tree with a chain with two blocks
 * @when trying to obtain the best chain with the second block
 * @then the second block hash is returned
 */
TEST_F(BlockTreeTest, GetBestChain_ShortChain) {
  EXPECT_CALL(*header_repo_, getBlockHeader(kLastFinalizedBlockId))
      .WillRepeatedly(Return(finalized_block_header_));

  auto target_hash = addHeaderToRepository(kLastFinalizedBlockId, 1337);

  EXPECT_OUTCOME_TRUE(best_info,
                      block_tree_->getBestContaining(target_hash, boost::none));
  ASSERT_EQ(best_info.hash, target_hash);
}

/**
 * @given a block tree with two branches-chains
 * @when trying to obtain the best chain containing the root of the split on
 two
 * chains
 * @then the longest chain with is returned
 */
TEST_F(BlockTreeTest, GetBestChain_TwoChains) {
  EXPECT_CALL(*header_repo_, getBlockHeader(kLastFinalizedBlockId))
      .WillRepeatedly(Return(finalized_block_header_));

  auto target_hash = addHeaderToRepository(kLastFinalizedBlockId, 1337);
  auto header00_hash = addHeaderToRepository(target_hash, 1338);
  addHeaderToRepository(header00_hash, 1339);
  auto header10_hash = addHeaderToRepository(target_hash, 1340);
  auto header11_hash = addHeaderToRepository(header10_hash, 1340);
  auto header12_hash = addHeaderToRepository(header11_hash, 1341);

  EXPECT_OUTCOME_TRUE(best_info,
                      block_tree_->getBestContaining(target_hash, boost::none));
  ASSERT_EQ(best_info.hash, header12_hash);
}

/**
 * @given a non-empty block tree
 * @when trying to obtain the best chain with a block, which number is past
 the
 * specified limit
 * @then TARGET_IS_PAST_MAX error is returned
 */
TEST_F(BlockTreeTest, GetBestChain_TargetPastMax) {
  EXPECT_CALL(*header_repo_, getBlockHeader(kLastFinalizedBlockId))
      .WillRepeatedly(Return(finalized_block_header_));

  auto target_hash = addHeaderToRepository(kLastFinalizedBlockId, 1337);

  EXPECT_OUTCOME_FALSE(err, block_tree_->getBestContaining(target_hash, 42));
  ASSERT_EQ(err, BlockTreeImpl::Error::TARGET_IS_PAST_MAX);
}
