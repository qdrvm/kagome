/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <chrono>

#include "consensus/timeline/impl/slots_util_impl.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/consensus/production_consensus_mock.hpp"
#include "mock/core/consensus/timeline/consensus_selector_mock.hpp"
#include "mock/core/runtime/babe_api_mock.hpp"
#include "mock/core/storage/spaced_storage_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using application::AppStateManagerMock;
using blockchain::BlockTreeMock;
using consensus::ConsensusSelectorMock;
using consensus::Duration;
using consensus::EpochLength;
using consensus::ProductionConsensusMock;
using consensus::SlotNumber;
using consensus::SlotsUtilImpl;
using runtime::BabeApiMock;
using storage::InMemoryStorage;
using storage::SpacedStorageMock;
using storage::trie::TrieStorageMock;

using std::chrono_literals::operator""ms;

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::ReturnRef;
using testing::ReturnRefOfCopy;

class SlotsUtilTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    EXPECT_CALL(app_state_manager, atPrepare(_)).WillOnce(Return());

    spaced_storage = std::make_shared<SpacedStorageMock>();
    persistent_storage = std::make_shared<InMemoryStorage>();
    EXPECT_CALL(*spaced_storage, getSpace(_))
        .WillRepeatedly(Return(persistent_storage));

    block_tree = std::make_shared<BlockTreeMock>();

    consensus_selector = std::make_shared<ConsensusSelectorMock>();
    production_consensus = std::make_shared<ProductionConsensusMock>();
    EXPECT_CALL(*consensus_selector, getProductionConsensus(_))
        .WillRepeatedly(Return(production_consensus));
    EXPECT_CALL(*production_consensus, getTimings())
        .WillRepeatedly(
            Invoke([&]() { return std::tuple(slot_duration, epoch_length); }));

    trie_storage = std::make_shared<TrieStorageMock>();
    babe_api = std::make_shared<BabeApiMock>();

    slots_util_ = std::make_shared<SlotsUtilImpl>(app_state_manager,
                                                  spaced_storage,
                                                  block_tree,
                                                  consensus_selector,
                                                  trie_storage,
                                                  babe_api);
  }

  AppStateManagerMock app_state_manager;
  std::shared_ptr<SpacedStorageMock> spaced_storage;
  std::shared_ptr<BlockTreeMock> block_tree;
  std::shared_ptr<ConsensusSelectorMock> consensus_selector;
  std::shared_ptr<TrieStorageMock> trie_storage;
  std::shared_ptr<BabeApiMock> babe_api;
  std::shared_ptr<InMemoryStorage> persistent_storage;
  std::shared_ptr<ProductionConsensusMock> production_consensus;
  Duration slot_duration;
  EpochLength epoch_length;

  std::shared_ptr<SlotsUtilImpl> slots_util_;
};

/**
 * @given current time
 * @when getCurrentSlot is called
 * @then compare slot estimations
 */
TEST_F(SlotsUtilTest, getCurrentSlot) {
  slot_duration = 12345ms;
  epoch_length = 321;

  slots_util_->prepare();

  auto time = std::chrono::system_clock::now();
  auto slot = slots_util_->timeToSlot(time);
  EXPECT_EQ(static_cast<SlotNumber>(time.time_since_epoch() / slot_duration),
            slot);
}
