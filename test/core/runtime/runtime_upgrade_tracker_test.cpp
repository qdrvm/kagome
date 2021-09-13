/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/runtime_upgrade_tracker_impl.hpp"

#include <gtest/gtest.h>

#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
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
    sub_engine_ =
        std::make_shared<kagome::primitives::events::ChainSubscriptionEngine>();
    tracker_ = std::make_unique<kagome::runtime::RuntimeUpgradeTrackerImpl>(
        header_repo_);
  }

 protected:
  std::unique_ptr<kagome::runtime::RuntimeUpgradeTrackerImpl> tracker_;
  std::shared_ptr<kagome::blockchain::BlockHeaderRepositoryMock> header_repo_;
  std::shared_ptr<kagome::blockchain::BlockTreeMock> block_tree_;
  std::shared_ptr<kagome::primitives::events::ChainSubscriptionEngine>
      sub_engine_;

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
  EXPECT_CALL(*header_repo_, getBlockHeader(kagome::primitives::BlockId{0}))
      .WillOnce(testing::Return(genesis_block_header));
  EXPECT_OUTCOME_TRUE(state, tracker_->getLastCodeUpdateState(block_42));
  ASSERT_EQ(state, "genesis_state_root"_hash256);
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
  EXPECT_OUTCOME_TRUE(state, tracker_->getLastCodeUpdateState(block_42));
  ASSERT_EQ(state, "block_42_state_root"_hash256);
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
  EXPECT_OUTCOME_TRUE(state, tracker_->getLastCodeUpdateState(block_2));
  ASSERT_EQ(state, "block_2_state_root"_hash256);

  EXPECT_CALL(*block_tree_, getLastFinalized())
      .WillRepeatedly(testing::Return(block_42));
  EXPECT_OUTCOME_TRUE(state42, tracker_->getLastCodeUpdateState(block_42));
  // picking 2 instead of 42 because that's the latest known upgrade
  ASSERT_EQ(state42, "block_2_state_root"_hash256);
}

TEST_F(RuntimeUpgradeTrackerTest, CorrectUpgradeScenario) {
  tracker_->subscribeToBlockchainEvents(sub_engine_, block_tree_);
  EXPECT_CALL(*block_tree_, getLastFinalized())
      .WillRepeatedly(testing::Return(makeBlockInfo(100500)));

  // first we execute block #1
  EXPECT_CALL(*header_repo_,
              getBlockHeader(kagome::primitives::BlockId{genesis_block.hash}))
      .WillRepeatedly(testing::Return(genesis_block_header));

  EXPECT_OUTCOME_TRUE(state_for_1,
                      tracker_->getLastCodeUpdateState(genesis_block));
  ASSERT_EQ(state_for_1, genesis_block_header.state_root);

  // then we upgrade in block #42
  auto block_41_header = makeBlockHeader(41);
  auto block_41 = makeBlockInfo(41);
  EXPECT_CALL(*header_repo_, getBlockHeader(kagome::primitives::BlockId{0}))
      .WillRepeatedly(testing::Return(genesis_block_header));
  EXPECT_CALL(*header_repo_,
              getBlockHeader(kagome::primitives::BlockId{block_42.hash}))
      .WillOnce(testing::Return(block_42_header));

  EXPECT_OUTCOME_TRUE(state_for_42, tracker_->getLastCodeUpdateState(block_41));
  ASSERT_EQ(state_for_42, genesis_block_header.state_root);
  // during execution of 42 we upgrade the code
  sub_engine_->notify(
      kagome::primitives::events::ChainEventType::kRuntimeVersion,
      kagome::primitives::events::RuntimeVersionEventParams{block_42.hash, {}});

  // then we execute block #43
  auto block_43 = makeBlockInfo(43);
  auto block_43_header = makeBlockHeader(43);
  EXPECT_CALL(*block_tree_, getChildren(block_41.hash))
      .WillRepeatedly(testing::Return(
          std::vector<kagome::primitives::BlockHash>{block_42.hash}));
  EXPECT_CALL(*header_repo_,
              getBlockHeader(kagome::primitives::BlockId{block_42.hash}))
      .WillRepeatedly(testing::Return(block_42_header));

  EXPECT_OUTCOME_TRUE(state_for_43, tracker_->getLastCodeUpdateState(block_42));
  ASSERT_EQ(state_for_43, block_42_header.state_root);

  // then block #44
  EXPECT_OUTCOME_TRUE(state_for_44, tracker_->getLastCodeUpdateState(block_43));
  ASSERT_EQ(state_for_44, block_42_header.state_root);
}

using RuntimeUpgradeTrackerDeathTest = RuntimeUpgradeTrackerTest;
