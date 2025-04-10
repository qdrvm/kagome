/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/runtime_upgrade_tracker_impl.hpp"

#include <gtest/gtest.h>

#include <qtils/test/outcome.hpp>

#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/storage/spaced_storage_mock.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::common::Hash256;
using std::string_literals::operator""s;

Hash256 makeHash(std::string_view str) {
  kagome::common::Hash256 hash{};
  std::copy_n(str.begin(), std::min(str.size(), 32ul), hash.rbegin());
  return hash;
}

kagome::primitives::BlockHeader makeBlockHeader(
    kagome::primitives::BlockNumber number, size_t fork = 0) {
  auto fork_number = (fork > 0 ? "f" + std::to_string(fork) : "");
  auto str_number = std::to_string(number) + fork_number;

  return kagome::primitives::BlockHeader{
      number,  // number
      makeHash(
          "block_"
          + (number > 0 ? std::to_string(number - 1) + fork_number : "genesis")
          + "_hash"),                                   // parent
      makeHash("block_" + str_number + "_state_root"),  // state_root
      makeHash("block_" + str_number + "_ext_root"),    // extrinsics_root
      {}                                                // digest
  };
}

kagome::primitives::BlockInfo makeBlockInfo(
    kagome::primitives::BlockNumber number, size_t fork = 0) {
  auto fork_number = (fork > 0 ? "f" + std::to_string(fork) : "");
  auto str_number = std::to_string(number) + fork_number;
  return kagome::primitives::BlockInfo{
      number,
      makeHash("block_" + (number > 0 ? str_number : "genesis") + "_hash")};
}

class RuntimeUpgradeTrackerTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers(soralog::Level::DEBUG);
  }

  void SetUp() override {
    block_tree_ = std::make_shared<kagome::blockchain::BlockTreeMock>();
    buffer_storage_ = std::make_shared<kagome::storage::InMemoryStorage>();
    storage_ = std::make_shared<kagome::storage::SpacedStorageMock>();

    EXPECT_CALL(*storage_, getSpace(::testing::_))
        .WillRepeatedly(::testing::Return(buffer_storage_));

    known_code_substitutes_ =
        std::make_shared<kagome::primitives::CodeSubstituteBlockIds>();
    sub_engine_ =
        std::make_shared<kagome::primitives::events::ChainSubscriptionEngine>();
    tracker_ = kagome::runtime::RuntimeUpgradeTrackerImpl::create(
                   storage_, known_code_substitutes_, block_tree_)
                   .value();
  }

 protected:
  std::unique_ptr<kagome::runtime::RuntimeUpgradeTrackerImpl> tracker_;
  std::shared_ptr<kagome::blockchain::BlockTreeMock> block_tree_;
  std::shared_ptr<kagome::primitives::events::ChainSubscriptionEngine>
      sub_engine_;
  std::shared_ptr<kagome::storage::BufferStorage> buffer_storage_;
  std::shared_ptr<kagome::storage::SpacedStorageMock> storage_;

  std::shared_ptr<kagome::primitives::CodeSubstituteBlockIds>
      known_code_substitutes_{};
  kagome::primitives::BlockInfo genesis_block{0, "block_genesis_hash"_hash256};
  kagome::primitives::BlockHeader genesis_block_header{
      0,                             // number
      ""_hash256,                    // parent
      "genesis_state_root"_hash256,  // state_root
      "genesis_ext_root"_hash256,    // extrinsics_root
      {}                             // digest
  };
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
 * THEN first encountered state is returned
 */
TEST_F(RuntimeUpgradeTrackerTest, NullBlockTree) {
  EXPECT_CALL(*block_tree_, getBlockHeader({block_42.hash}))
      .WillOnce(testing::Return(block_42_header));
  ASSERT_OUTCOME_SUCCESS(state, tracker_->getLastCodeUpdateState(block_42));
  ASSERT_EQ(state, block_42_header.state_root);
}

/*
 * GIVEN initialized upgrade tracker with empty upgrades list
 * WHEN querying the latest code update from it
 * THEN the state of the block for which the latest update has been queried is
 * returned
 */
TEST_F(RuntimeUpgradeTrackerTest, EmptyUpdatesCache) {
  tracker_->subscribeToBlockchainEvents(sub_engine_);

  EXPECT_CALL(*block_tree_, getBlockHeader(block_42.hash))
      .WillOnce(testing::Return(block_42_header));

  ASSERT_OUTCOME_SUCCESS(state, tracker_->getLastCodeUpdateState(block_42));
  ASSERT_EQ(state, block_42_header.state_root);
}

/*
 * GIVEN initialized upgrade tracker with the first update reported for genesis
 * WHEN querying the latest code update for block #1
 * THEN genesis state is returned
 */
TEST_F(RuntimeUpgradeTrackerTest, AutoUpgradeAfterEmpty) {
  tracker_->subscribeToBlockchainEvents(sub_engine_);

  EXPECT_CALL(*block_tree_, getBlockHeader(block_2.hash))
      .WillRepeatedly(testing::Return(block_2_header));
  ASSERT_OUTCOME_SUCCESS(state, tracker_->getLastCodeUpdateState(block_2));
  ASSERT_EQ(state, block_2_header.state_root);

  EXPECT_CALL(*block_tree_, getLastFinalized())
      .WillRepeatedly(testing::Return(block_42));
  ASSERT_OUTCOME_SUCCESS(state42, tracker_->getLastCodeUpdateState(block_42));
  // picking 2 instead of 42 because that's the latest known upgrade
  ASSERT_EQ(state42, block_2_header.state_root);
}

TEST_F(RuntimeUpgradeTrackerTest, CorrectUpgradeScenario) {
  tracker_->subscribeToBlockchainEvents(sub_engine_);
  EXPECT_CALL(*block_tree_, getLastFinalized())
      .WillRepeatedly(testing::Return(makeBlockInfo(100500)));

  // first we execute block #1
  EXPECT_CALL(*block_tree_, getBlockHeader(genesis_block.hash))
      .WillRepeatedly(testing::Return(genesis_block_header));

  ASSERT_OUTCOME_SUCCESS(state1,
                         tracker_->getLastCodeUpdateState(genesis_block));
  ASSERT_EQ(state1, genesis_block_header.state_root);

  // then we upgrade in block #42
  auto block_41_header = makeBlockHeader(41);
  auto block_41 = makeBlockInfo(41);
  EXPECT_CALL(*block_tree_, getBlockHeader(genesis_block.hash))
      .WillRepeatedly(testing::Return(genesis_block_header));
  EXPECT_CALL(*block_tree_, getBlockHeader(block_42.hash))
      .WillRepeatedly(testing::Return(block_42_header));

  ASSERT_OUTCOME_SUCCESS(state42, tracker_->getLastCodeUpdateState(block_41));
  ASSERT_EQ(state42, genesis_block_header.state_root);
  // during execution of 42 we upgrade the code
  sub_engine_->notify(
      kagome::primitives::events::ChainEventType::kNewRuntime,
      kagome::primitives::events::NewRuntimeEventParams{block_42.hash});

  // then we execute block #43
  auto block_43 = makeBlockInfo(43);
  auto block_43_header = makeBlockHeader(43);
  EXPECT_CALL(*block_tree_, getChildren(block_41.hash))
      .WillRepeatedly(testing::Return(
          std::vector<kagome::primitives::BlockHash>{block_42.hash}));
  EXPECT_CALL(*block_tree_, getBlockHeader(block_42.hash))
      .WillRepeatedly(testing::Return(block_42_header));

  ASSERT_OUTCOME_SUCCESS(state43, tracker_->getLastCodeUpdateState(block_42));
  ASSERT_EQ(state43, block_42_header.state_root);

  // then block #44
  ASSERT_OUTCOME_SUCCESS(state44, tracker_->getLastCodeUpdateState(block_43));
  ASSERT_EQ(state44, block_42_header.state_root);
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

  tracker_->subscribeToBlockchainEvents(sub_engine_);
  auto block1 = makeBlockInfo(5200000);  // took a block before code update!!!
  auto block1_header = makeBlockHeader(5200000);
  EXPECT_CALL(*block_tree_, getBlockHeader(block1.hash))
      .WillRepeatedly(testing::Return(block1_header));
  sub_engine_->notify(
      kagome::primitives::events::ChainEventType::kNewRuntime,
      kagome::primitives::events::NewRuntimeEventParams{block1.hash});

  auto block2 = makeBlockInfo(5203203);
  auto block2_header = makeBlockHeader(5203203);
  EXPECT_CALL(*block_tree_, getBlockHeader(block2.hash))
      .WillRepeatedly(testing::Return(block2_header));
  known_code_substitutes_.reset(
      new kagome::primitives::CodeSubstituteBlockIds{{block2.number}});

  // reset tracker
  tracker_ = kagome::runtime::RuntimeUpgradeTrackerImpl::create(
                 storage_, known_code_substitutes_, block_tree_)
                 .value();
  tracker_->subscribeToBlockchainEvents(sub_engine_);

  ASSERT_OUTCOME_SUCCESS(state2, tracker_->getLastCodeUpdateState(block2));
  ASSERT_EQ(state2, block2_header.state_root);

  // reset tracker
  tracker_ = kagome::runtime::RuntimeUpgradeTrackerImpl::create(
                 storage_, known_code_substitutes_, block_tree_)
                 .value();
  tracker_->subscribeToBlockchainEvents(sub_engine_);

  auto block3 = makeBlockInfo(5203204);
  ASSERT_OUTCOME_SUCCESS(state3, tracker_->getLastCodeUpdateState(block3));
  ASSERT_EQ(state3, block2_header.state_root);
}

TEST_F(RuntimeUpgradeTrackerTest, UpgradeAfterCodeSubstitute) {
  EXPECT_CALL(*block_tree_, getLastFinalized())
      .WillRepeatedly(testing::Return(makeBlockInfo(5661184)));
  EXPECT_CALL(*block_tree_, hasDirectChain(testing::_, testing::_))
      .WillRepeatedly(testing::Return(true));

  tracker_ = kagome::runtime::RuntimeUpgradeTrackerImpl::create(
                 storage_, known_code_substitutes_, block_tree_)
                 .value();
  tracker_->subscribeToBlockchainEvents(sub_engine_);

  auto block1 = makeBlockInfo(5203203);
  auto block1_header = makeBlockHeader(5203203);
  known_code_substitutes_.reset(
      new kagome::primitives::CodeSubstituteBlockIds({block1.number}));

  EXPECT_CALL(*block_tree_, getBlockHeader(block1.hash))
      .WillOnce(testing::Return(block1_header));
  EXPECT_OUTCOME_SUCCESS(tracker_->getLastCodeUpdateState(block1));

  // @see https://polkadot.subscan.io/event?module=system&event=codeupdated
  auto block2 = makeBlockInfo(5661442);
  auto block2_header = makeBlockHeader(5661442);
  EXPECT_CALL(*block_tree_, getBlockHeader(block2.hash))
      .WillRepeatedly(testing::Return(block2_header));
  sub_engine_->notify(
      kagome::primitives::events::ChainEventType::kNewRuntime,
      kagome::primitives::events::NewRuntimeEventParams{block2.hash});

  ASSERT_OUTCOME_SUCCESS(state2, tracker_->getLastCodeUpdateState(block2));
  ASSERT_EQ(state2, block2_header.state_root);

  auto block3 = makeBlockInfo(5661443);
  ASSERT_OUTCOME_SUCCESS(state3, tracker_->getLastCodeUpdateState(block3));
  ASSERT_EQ(state3, block2_header.state_root);
}

TEST_F(RuntimeUpgradeTrackerTest, OrphanBlock) {
  tracker_->subscribeToBlockchainEvents(sub_engine_);
  // suppose we have two forks
  //  / - 33f2
  // 32 - 33f1 - 34f1
  // with an empty upgrade tracker
  EXPECT_CALL(*block_tree_, getLastFinalized())
      .WillRepeatedly(testing::Return(makeBlockInfo(32)));

  // and then we receive 34f2 with a runtime upgrade
  auto block_34f2 = makeBlockInfo(34, 2);
  auto block_34f2_header = makeBlockHeader(34, 2);
  EXPECT_CALL(*block_tree_, getBlockHeader(block_34f2.hash))
      .WillRepeatedly(testing::Return(block_34f2_header));
  sub_engine_->notify(
      kagome::primitives::events::ChainEventType::kNewRuntime,
      kagome::primitives::events::NewRuntimeEventParams{block_34f2.hash});

  // and then we receive 35f1 and query the latest runtime for it
  auto block_35f1 = makeBlockInfo(35, 1);
  auto block_35f1_header = makeBlockHeader(35, 1);
  EXPECT_CALL(*block_tree_, getBlockHeader(block_35f1.hash))
      .WillRepeatedly(testing::Return(block_35f1_header));

  EXPECT_CALL(*block_tree_, hasDirectChain(block_34f2.hash, block_35f1.hash))
      .WillOnce(testing::Return(false));
  ASSERT_OUTCOME_SUCCESS(state_for_35f1,
                         tracker_->getLastCodeUpdateState(block_35f1));

  // we have no information on upgrades, related to this block, so we fall back
  // to returning its state root
  ASSERT_EQ(state_for_35f1, block_35f1_header.state_root);

  auto block_33f1 = makeBlockInfo(33, 1);
  auto block_33f1_header = makeBlockHeader(33, 1);
  EXPECT_CALL(*block_tree_, getBlockHeader(block_33f1.hash))
      .WillRepeatedly(testing::Return(block_33f1_header));
  sub_engine_->notify(
      kagome::primitives::events::ChainEventType::kNewRuntime,
      kagome::primitives::events::NewRuntimeEventParams{block_33f1.hash});
  EXPECT_CALL(*block_tree_, hasDirectChain(block_34f2.hash, block_35f1.hash))
      .WillOnce(testing::Return(false));
  EXPECT_CALL(*block_tree_, hasDirectChain(block_33f1.hash, block_35f1.hash))
      .WillOnce(testing::Return(true));
  ASSERT_OUTCOME_SUCCESS(state_for_35f1_again,
                         tracker_->getLastCodeUpdateState(block_35f1));

  // now we pick the runtime upgrade
  ASSERT_EQ(state_for_35f1_again, block_33f1_header.state_root);
}

using RuntimeUpgradeTrackerDeathTest = RuntimeUpgradeTrackerTest;
