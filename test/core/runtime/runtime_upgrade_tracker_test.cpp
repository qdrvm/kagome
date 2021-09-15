/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/runtime_upgrade_tracker_impl.hpp"

#include <gtest/gtest.h>

#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::common::Hash256;
using std::string_literals::operator""s;

Hash256 makeHash(std::string_view str) {
  kagome::common::Hash256 hash{};
  std::copy_n(str.begin(), std::min(str.size(), 32ul), hash.rbegin());
  return hash;
}

kagome::primitives::BlockHeader makeBlockHeader(
    kagome::primitives::BlockNumber number) {
  auto str_number = std::to_string(number);
  return kagome::primitives::BlockHeader{
      makeHash("block_" + (number > 0 ? std::to_string(number - 1) : "genesis")
               + "_hash"),
      number,
      makeHash("block_" + str_number + "_state_root"),
      makeHash("block_" + str_number + "_ext_root"),
      {}};
}

kagome::primitives::BlockInfo makeBlockInfo(
    kagome::primitives::BlockNumber number) {
  auto str_number = std::to_string(number);
  return kagome::primitives::BlockInfo{
      number,
      makeHash("block_" + (number > 0 ? std::to_string(number) : "genesis")
               + "_hash")};
}

class RuntimeUpgradeTrackerTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers(soralog::Level::DEBUG);
  }

  void SetUp() override {
    header_repo_ =
        std::make_shared<kagome::blockchain::BlockHeaderRepositoryMock>();
    block_tree_ = std::make_shared<kagome::blockchain::BlockTreeMock>();
    sub_engine_ = std::make_shared<
        kagome::primitives::events::StorageSubscriptionEngine>();
    storage_ = std::make_shared<kagome::storage::InMemoryStorage>();
    tracker_ = std::make_unique<kagome::runtime::RuntimeUpgradeTrackerImpl>(
        header_repo_, storage_, code_substitutes_);
  }

 protected:
  std::unique_ptr<kagome::runtime::RuntimeUpgradeTrackerImpl> tracker_;
  std::shared_ptr<kagome::blockchain::BlockHeaderRepositoryMock> header_repo_;
  std::shared_ptr<kagome::blockchain::BlockTreeMock> block_tree_;
  std::shared_ptr<kagome::primitives::events::StorageSubscriptionEngine>
      sub_engine_;
  std::shared_ptr<kagome::storage::BufferStorage> storage_;

  kagome::application::CodeSubstitutes code_substitutes_{};
  kagome::primitives::BlockInfo genesis_block{0, "block_genesis_hash"_hash256};
  kagome::primitives::BlockHeader genesis_block_header{
      ""_hash256,
      0,
      "genesis_state_root"_hash256,
      "genesis_ext_root"_hash256,
      {}};
  kagome::primitives::BlockInfo block_1 = makeBlockInfo(1);
  kagome::primitives::BlockHeader block_1_header = makeBlockHeader(1);
  kagome::primitives::BlockInfo block_2 = makeBlockInfo(2);
  kagome::primitives::BlockHeader block_2_header = makeBlockHeader(2);
  kagome::primitives::BlockInfo block_42 = makeBlockInfo(42);
  kagome::primitives::BlockHeader block_42_header = makeBlockHeader(42);
};

/*
 * GIVEN uninitialized upgrade tracker
 * WHEN querying the latest code update from it
 * THEN genesis state is returned
 */
TEST_F(RuntimeUpgradeTrackerTest, NullBlockTree) {
  EXPECT_OUTCOME_TRUE(id, tracker_->getRuntimeChangeBlock(block_42));
  ASSERT_EQ(boost::get<kagome::primitives::BlockNumber>(id), 0);
}

/*
 * GIVEN initialized upgrade tracker with empty upgrades list
 * WHEN querying the latest code update from it
 * THEN the state of the block for which the latest update has been queried is
 * returned
 */
TEST_F(RuntimeUpgradeTrackerTest, EmptyUpdatesCache) {
  tracker_->subscribeToBlockchainEvents(sub_engine_, block_tree_);

  EXPECT_CALL(*header_repo_,
              getBlockHeader(kagome::primitives::BlockId{block_42.hash}))
      .WillOnce(testing::Return(block_42_header));
  EXPECT_OUTCOME_TRUE(id, tracker_->getRuntimeChangeBlock(block_42));
  ASSERT_EQ(boost::get<kagome::primitives::BlockHash>(id), block_42.hash);
}

/*
 * GIVEN initialized upgrade tracker with the first update reported for genesis
 * WHEN querying the latest code update for block #1
 * THEN genesis state is returned
 */
TEST_F(RuntimeUpgradeTrackerTest, AutoUpgradeAfterEmpty) {
  tracker_->subscribeToBlockchainEvents(sub_engine_, block_tree_);

  EXPECT_CALL(*header_repo_,
              getBlockHeader(kagome::primitives::BlockId{block_2.hash}))
      .WillRepeatedly(testing::Return(block_2_header));
  EXPECT_OUTCOME_TRUE(id2, tracker_->getRuntimeChangeBlock(block_2));
  ASSERT_EQ(boost::get<kagome::primitives::BlockHash>(id2), block_2.hash);

  EXPECT_CALL(*block_tree_, getLastFinalized())
      .WillRepeatedly(testing::Return(block_42));
  EXPECT_CALL(*block_tree_, getChildren(block_1.hash))
      .WillOnce(testing::Return(
          std::vector<kagome::primitives::BlockHash>{block_2.hash}));
  EXPECT_OUTCOME_TRUE(id42, tracker_->getRuntimeChangeBlock(block_42));
  // picking 2 instead of 42 because that's the latest known upgrade
  ASSERT_EQ(boost::get<kagome::primitives::BlockHash>(id42), block_2.hash);
}

TEST_F(RuntimeUpgradeTrackerTest, CorrectUpgradeScenario) {
  tracker_->subscribeToBlockchainEvents(sub_engine_, block_tree_);
  EXPECT_CALL(*block_tree_, getLastFinalized())
      .WillRepeatedly(testing::Return(makeBlockInfo(100500)));

  // first we execute block #1
  EXPECT_CALL(*header_repo_,
              getBlockHeader(kagome::primitives::BlockId{genesis_block.hash}))
      .WillRepeatedly(testing::Return(genesis_block_header));

  EXPECT_OUTCOME_TRUE(id1, tracker_->getRuntimeChangeBlock(genesis_block));
  ASSERT_EQ(boost::get<kagome::primitives::BlockHash>(id1), genesis_block.hash);

  // then we upgrade in block #42
  auto block_41_header = makeBlockHeader(41);
  auto block_41 = makeBlockInfo(41);
  EXPECT_CALL(*header_repo_, getBlockHeader(kagome::primitives::BlockId{0}))
      .WillRepeatedly(testing::Return(genesis_block_header));
  EXPECT_CALL(*header_repo_, getNumberByHash(block_41.hash))
      .WillOnce(testing::Return(41));

  EXPECT_OUTCOME_TRUE(id42, tracker_->getRuntimeChangeBlock(block_41));
  ASSERT_EQ(boost::get<kagome::primitives::BlockNumber>(id42), 0);
  // during execution of 42 we upgrade the code
  sub_engine_->notify(":code"_buf, "12345"_buf, block_41.hash);

  // then we execute block #43
  auto block_43 = makeBlockInfo(43);
  auto block_43_header = makeBlockHeader(43);
  EXPECT_CALL(*block_tree_, getChildren(block_41.hash))
      .WillRepeatedly(testing::Return(
          std::vector<kagome::primitives::BlockHash>{block_42.hash}));
  EXPECT_CALL(*header_repo_,
              getBlockHeader(kagome::primitives::BlockId{block_42.hash}))
      .WillRepeatedly(testing::Return(block_42_header));

  EXPECT_OUTCOME_TRUE(id43, tracker_->getRuntimeChangeBlock(block_42));
  ASSERT_EQ(boost::get<kagome::primitives::BlockNumber>(id43),
            genesis_block.number);

  // then block #44
  EXPECT_OUTCOME_TRUE(id44, tracker_->getRuntimeChangeBlock(block_43));
  ASSERT_EQ(boost::get<kagome::primitives::BlockHash>(id44), block_42.hash);
}

/*
 * GIVEN real usecase from polkadot chain with code substitute at #5203203 and
 * code update at #5661442
 * WHEN querying the latest code update for block #1
 * THEN genesis state is returned
 */
TEST_F(RuntimeUpgradeTrackerTest, CodeSubstituteAndStore) {
  EXPECT_CALL(*block_tree_, getLastFinalized())
      .WillRepeatedly(testing::Return(makeBlockInfo(5203205)));

  tracker_->subscribeToBlockchainEvents(sub_engine_, block_tree_);
  auto block1 = makeBlockInfo(5200000);  // took a block before code update!!!
  EXPECT_CALL(*header_repo_, getNumberByHash(block1.hash))
      .WillOnce(testing::Return(5200000));
  sub_engine_->notify(":code"_buf, "5200001"_buf, block1.hash);

  auto block3 = makeBlockInfo(5203203);
  code_substitutes_ = {{block3.hash, "5203203"_buf}};

  // reset tracker
  tracker_ = std::make_unique<kagome::runtime::RuntimeUpgradeTrackerImpl>(
      header_repo_, storage_, code_substitutes_);
  tracker_->subscribeToBlockchainEvents(sub_engine_, block_tree_);

  auto block2 = makeBlockInfo(5200001);
  EXPECT_CALL(*block_tree_, getChildren(block1.hash))
      .WillOnce(testing::Return(
          std::vector<kagome::primitives::BlockHash>{block2.hash}));
  EXPECT_CALL(*header_repo_,
              getBlockHeader(kagome::primitives::BlockId{block2.hash}))
      .WillOnce(testing::Return(makeBlockHeader(5200001)));

  EXPECT_OUTCOME_TRUE(id3, tracker_->getRuntimeChangeBlock(block3));
  ASSERT_EQ(boost::get<kagome::primitives::BlockHash>(id3), block2.hash);

  // reset tracker
  tracker_ = std::make_unique<kagome::runtime::RuntimeUpgradeTrackerImpl>(
      header_repo_, storage_, code_substitutes_);
  tracker_->subscribeToBlockchainEvents(sub_engine_, block_tree_);

  auto block4 = makeBlockInfo(5203204);
  EXPECT_OUTCOME_TRUE(id4, tracker_->getRuntimeChangeBlock(block4));
  ASSERT_EQ(boost::get<kagome::primitives::BlockHash>(id4), block3.hash);
}

TEST_F(RuntimeUpgradeTrackerTest, UpgradeAfterCodeSubstitute) {
  EXPECT_CALL(*block_tree_, getLastFinalized())
      .WillRepeatedly(testing::Return(makeBlockInfo(5661184)));
  EXPECT_CALL(*block_tree_, hasDirectChain(testing::_, testing::_))
      .WillRepeatedly(testing::Return(true));

  tracker_ = std::make_unique<kagome::runtime::RuntimeUpgradeTrackerImpl>(
      header_repo_, storage_, code_substitutes_);
  tracker_->subscribeToBlockchainEvents(sub_engine_, block_tree_);

  auto block1 = makeBlockInfo(5203203);
  code_substitutes_ = {{block1.hash, "5203203"_buf}};

  EXPECT_CALL(*header_repo_,
              getBlockHeader(kagome::primitives::BlockId{block1.hash}))
      .WillRepeatedly(testing::Return(makeBlockHeader(block1.number)));
  EXPECT_OUTCOME_TRUE(id1, tracker_->getRuntimeChangeBlock(block1));

  // @see https://polkadot.subscan.io/event?module=system&event=codeupdated
  auto block2 = makeBlockInfo(5661441);  // took a block before code update!!!
  EXPECT_CALL(*header_repo_, getNumberByHash(block2.hash))
      .WillOnce(testing::Return(5661441));
  sub_engine_->notify(":code"_buf, "5661442"_buf, block2.hash);

  auto block3 = makeBlockInfo(5661442);
  /*
   * EXPECT_CALL(*block_tree_, getChildren(block2.hash))
   *     .WillOnce(testing::Return(
   *         std::vector<kagome::primitives::BlockHash>{block3.hash}));
   * EXPECT_CALL(*header_repo_,
   *             getBlockHeader(kagome::primitives::BlockId{block3.hash}))
   *     .WillOnce(testing::Return(makeBlockHeader(5661442)));
   */

  EXPECT_OUTCOME_TRUE(id3, tracker_->getRuntimeChangeBlock(block3));
  ASSERT_EQ(boost::get<kagome::primitives::BlockHash>(id3), block1.hash);

  /*
   * auto block4 = makeBlockInfo(5661443);
   * EXPECT_OUTCOME_TRUE(id4, tracker_->getRuntimeChangeBlock(block4));
   * ASSERT_EQ(boost::get<kagome::primitives::BlockHash>(id4), block3.hash);
   */
}

using RuntimeUpgradeTrackerDeathTest = RuntimeUpgradeTrackerTest;
