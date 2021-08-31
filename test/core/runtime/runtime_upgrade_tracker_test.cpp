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
      makeHash("block_" + (number > 0 ? std::to_string(number) : "genesis")
               + "_hash"),
      number,
      makeHash("block_" + str_number + "_state_root"),
      makeHash("block_" + str_number + "_ext_root"),
      {}};
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
    tracker_ = std::make_unique<kagome::runtime::RuntimeUpgradeTrackerImpl>(
        header_repo_);
  }

 protected:
  std::unique_ptr<kagome::runtime::RuntimeUpgradeTrackerImpl> tracker_;
  std::shared_ptr<kagome::blockchain::BlockHeaderRepositoryMock> header_repo_;
  std::shared_ptr<kagome::blockchain::BlockTreeMock> block_tree_;
  std::shared_ptr<kagome::primitives::events::StorageSubscriptionEngine>
      sub_engine_;

  kagome::primitives::BlockInfo genesis_block{0, "block_genesis_hash"_hash256};
  kagome::primitives::BlockHeader genesis_block_header{
      ""_hash256,
      0,
      "genesis_state_root"_hash256,
      "genesis_ext_root"_hash256,
      {}};
  kagome::primitives::BlockInfo block_1{1, "block1_hash"_hash256};
  kagome::primitives::BlockHeader block_1_header = makeBlockHeader(1);
  kagome::primitives::BlockInfo block_2{2, "block2_hash"_hash256};
  kagome::primitives::BlockHeader block_2_header = makeBlockHeader(2);
  kagome::primitives::BlockInfo block_42{42, "block42_hash"_hash256};
  kagome::primitives::BlockHeader block_42_header = makeBlockHeader(42);
};

TEST_F(RuntimeUpgradeTrackerTest, NullBlockTree) {
  EXPECT_CALL(*header_repo_, getBlockHeader(kagome::primitives::BlockId{0}))
      .WillOnce(testing::Return(genesis_block_header));
  EXPECT_OUTCOME_TRUE(state, tracker_->getLastCodeUpdateState(block_42));
  ASSERT_EQ(state, "genesis_state_root"_hash256);
}

TEST_F(RuntimeUpgradeTrackerTest, EmptyUpdatesCache) {
  tracker_->subscribeToBlockchainEvents(sub_engine_, block_tree_);

  EXPECT_CALL(*header_repo_,
              getBlockHeader(kagome::primitives::BlockId{block_42.hash}))
      .WillOnce(testing::Return(block_42_header));
  EXPECT_OUTCOME_TRUE(state, tracker_->getLastCodeUpdateState(block_42));
  ASSERT_EQ(state, "block_42_state_root"_hash256);
}

TEST_F(RuntimeUpgradeTrackerTest, CorrectFirstUpgrade) {
  tracker_->subscribeToBlockchainEvents(sub_engine_, block_tree_);

  EXPECT_CALL(*header_repo_, getNumberByHash("block_genesis_hash"_hash256))
      .WillOnce(testing::Return(0));

  sub_engine_->notify(":code"_buf, "12345"_buf, "block_genesis_hash"_hash256);

  EXPECT_CALL(*block_tree_, getLastFinalized())
      .WillOnce(testing::Return(block_2));
  EXPECT_OUTCOME_TRUE(state, tracker_->getLastCodeUpdateState(block_42));
  ASSERT_EQ(state, "genesis_state_root"_hash256);
}

TEST_F(RuntimeUpgradeTrackerTest, FirstBlockUpgrade) {}

using RuntimeUpgradeTrackerDeathTest = RuntimeUpgradeTrackerTest;

TEST_F(RuntimeUpgradeTrackerDeathTest, WrongFirstUpgrade) {
  tracker_->subscribeToBlockchainEvents(sub_engine_, block_tree_);

  ASSERT_DEATH(
      {
        EXPECT_CALL(*header_repo_, getNumberByHash("block_42_hash"_hash256))
            .WillOnce(testing::Return(42));

        sub_engine_->notify(":code"_buf, "12345"_buf, "block_42_hash"_hash256);
      },
      "");
}
