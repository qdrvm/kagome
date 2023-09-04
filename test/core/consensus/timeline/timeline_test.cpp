/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <mock/libp2p/basic/scheduler_mock.hpp>

#include "consensus/babe/types/babe_block_header.hpp"
#include "consensus/babe/types/seal.hpp"
#include "consensus/babe/types/slot.hpp"
#include "consensus/timeline/impl/timeline_impl.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/clock/clock_mock.hpp"
#include "mock/core/consensus/grandpa/grandpa_mock.hpp"
#include "mock/core/consensus/production_consensus_mock.hpp"
#include "mock/core/consensus/timeline/consensus_selector_mock.hpp"
#include "mock/core/consensus/timeline/consistency_keeper_mock.hpp"
#include "mock/core/consensus/timeline/slots_util_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/network/block_announce_transmitter_mock.hpp"
#include "mock/core/network/synchronizer_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "primitives/digest.hpp"
#include "primitives/event_types.hpp"
#include "testutil/lazy.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::application::AppConfigurationMock;
using kagome::application::AppStateManagerMock;
using kagome::blockchain::BlockTreeMock;
using kagome::clock::SystemClockMock;
using kagome::common::Buffer;
using kagome::consensus::ConsensusSelectorMock;
using kagome::consensus::ConsistencyKeeperMock;
using kagome::consensus::Duration;
using kagome::consensus::EpochDescriptor;
using kagome::consensus::EpochLength;
using kagome::consensus::ProductionConsensusMock;
using kagome::consensus::SlotNumber;
using kagome::consensus::SlotsUtilMock;
using kagome::consensus::SyncState;
using kagome::consensus::TimelineImpl;
using kagome::consensus::TimePoint;
using kagome::consensus::ValidatorStatus;
using kagome::consensus::babe::BabeBlockHeader;
using kagome::consensus::babe::Seal;
using kagome::consensus::babe::SlotType;
using kagome::consensus::grandpa::GrandpaMock;
using kagome::crypto::HasherMock;
using kagome::network::BlockAnnounceTransmitterMock;
using kagome::network::SynchronizerMock;
using kagome::network::WarpProtocol;  // TODO need to mock this
using kagome::network::WarpSync;      // TODO need to mock this
using kagome::primitives::Block;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockInfo;
using kagome::primitives::BlockNumber;
using kagome::primitives::Digest;
using kagome::primitives::Extrinsic;
using kagome::primitives::PreRuntime;
using kagome::primitives::detail::DigestItemCommon;
using kagome::primitives::events::BabeStateSubscriptionEngine;
using kagome::primitives::events::ChainSubscriptionEngine;
using kagome::runtime::CoreMock;
using kagome::storage::trie::TrieStorageMock;
using libp2p::basic::SchedulerMock;

using Seal = kagome::consensus::babe::Seal;
using SealDigest = kagome::primitives::Seal;

using testing::_;
using testing::A;
using testing::Invoke;
using testing::Mock;
using testing::Ref;
using testing::Return;
using testing::WithArg;
using namespace std::chrono_literals;

static Digest make_digest(SlotNumber slot) {
  Digest digest;

  BabeBlockHeader babe_header{
      .slot_assignment_type = SlotType::SecondaryPlain,
      .authority_index = 0,
      .slot_number = slot,
  };
  Buffer encoded_header{scale::encode(babe_header).value()};
  digest.emplace_back(
      PreRuntime{kagome::primitives::kBabeEngineId, encoded_header});

  Seal seal{};
  Buffer encoded_seal{scale::encode(seal).value()};
  digest.emplace_back(
      SealDigest{kagome::primitives::kBabeEngineId, encoded_seal});

  return digest;
};

class TimelineTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    app_state_manager = std::make_shared<AppStateManagerMock>();

    Duration slot_duration = 6s;
    EpochLength epoch_length = 200;

    slots_util = std::make_shared<SlotsUtilMock>();
    ON_CALL(*slots_util, slotDuration()).WillByDefault(Return(slot_duration));
    ON_CALL(*slots_util, epochLength()).WillByDefault(Return(epoch_length));
    ON_CALL(*slots_util, timeToSlot(_))
        .WillByDefault(WithArg<0>(Invoke([slot_duration](auto time) {
          return time.time_since_epoch() / slot_duration;
        })));
    ON_CALL(*slots_util, slotToEpochDescriptor(_, _))
        .WillByDefault(WithArg<1>(Invoke([epoch_length](auto slot) {
          return EpochDescriptor{slot / epoch_length,
                                 (slot / epoch_length) * epoch_length};
        })));

    block_tree = std::make_shared<BlockTreeMock>();
    EXPECT_CALL(*block_tree, bestLeaf()).WillRepeatedly(Return(best_block));
    EXPECT_CALL(*block_tree, getLastFinalized())
        .WillRepeatedly(Return(best_block));
    EXPECT_CALL(*block_tree, getBlockHeader(best_block_hash))
        .WillRepeatedly(Return(best_block_header));

    consensus_selector = std::make_shared<ConsensusSelectorMock>();
    production_consensus = std::make_shared<ProductionConsensusMock>();
    EXPECT_CALL(*consensus_selector, getProductionConsensus(_))
        .WillRepeatedly(Return(production_consensus));
    EXPECT_CALL(*production_consensus, getTimings())
        .WillRepeatedly(
            Invoke([&]() { return std::tuple(slot_duration, epoch_length); }));

    trie_storage = std::make_shared<TrieStorageMock>();
    synchronizer = std::make_shared<SynchronizerMock>();
    hasher = std::make_shared<HasherMock>();
    block_announce_transmitter =
        std::make_shared<BlockAnnounceTransmitterMock>();
    // warp_sync = std::make_shared<WarpSync>();
    // warp_protocol = std::make_shared<WarpProtocol>();
    justification_observer = std::make_shared<GrandpaMock>();
    consistency_keeper = std::make_shared<ConsistencyKeeperMock>();
    scheduler = std::make_shared<SchedulerMock>();
    core_api = std::make_shared<CoreMock>();
    chain_sub_engine = std::make_shared<ChainSubscriptionEngine>();
    state_sub_engine = std::make_shared<BabeStateSubscriptionEngine>();

    timeline = std::make_shared<TimelineImpl>(
        app_config,
        app_state_manager,
        clock,
        slots_util,
        block_tree,
        consensus_selector,
        trie_storage,
        synchronizer,
        hasher,
        block_announce_transmitter,
        warp_sync,
        testutil::sptr_to_lazy<WarpProtocol>(warp_protocol),
        justification_observer,
        consistency_keeper,
        scheduler,
        chain_sub_engine,
        state_sub_engine,
        core_api);
  }

  AppConfigurationMock app_config;
  std::shared_ptr<AppStateManagerMock> app_state_manager;
  SystemClockMock clock;
  std::shared_ptr<SlotsUtilMock> slots_util;
  std::shared_ptr<BlockTreeMock> block_tree;
  std::shared_ptr<ConsensusSelectorMock> consensus_selector;
  std::shared_ptr<TrieStorageMock> trie_storage;
  std::shared_ptr<SynchronizerMock> synchronizer;
  std::shared_ptr<HasherMock> hasher;
  std::shared_ptr<BlockAnnounceTransmitterMock> block_announce_transmitter;
  std::shared_ptr<WarpSync> warp_sync;
  std::shared_ptr<WarpProtocol> warp_protocol;
  std::shared_ptr<GrandpaMock> justification_observer;
  std::shared_ptr<ConsistencyKeeperMock> consistency_keeper;
  std::shared_ptr<SchedulerMock> scheduler;
  std::shared_ptr<ChainSubscriptionEngine> chain_sub_engine;
  std::shared_ptr<BabeStateSubscriptionEngine> state_sub_engine;
  std::shared_ptr<CoreMock> core_api;

  std::shared_ptr<TimelineImpl> timeline;

  std::shared_ptr<ProductionConsensusMock> production_consensus;

  BlockHash best_block_hash = "block#0"_hash256;
  BlockNumber best_block_number = 0u;
  BlockHeader best_block_header{
      best_block_number,           // number
      {},                          // parent
      "state_root#0"_hash256,      // state_root
      "extrinsic_root#0"_hash256,  // extrinsic_root
      {}                           // digest
  };

  BlockInfo best_block{best_block_number, best_block_hash};

  BlockHeader block_header{
      best_block_number + 1,       // number
      best_block_hash,             // parent
      "state_root#1"_hash256,      // state_root
      "extrinsic_root#1"_hash256,  // extrinsic_root
      make_digest(0)               // digest
  };

  Extrinsic extrinsic{{1, 2, 3}};
  Block created_block{block_header, {extrinsic}};

  BlockHash created_block_hash{"block#1"_hash256};
};

/**
 * @given start timeline
 * @when consensus returns we are not validator
 * @then we has not synchronized and waiting announce or incoming stream
 */
TEST_F(TimelineTest, NonValidator) {
  EXPECT_CALL(clock, now());
  EXPECT_CALL(*slots_util, slotFinishTime(_)).Times(testing::AnyNumber());
  EXPECT_CALL(*block_tree, bestLeaf()).WillRepeatedly(Return(best_block));
  EXPECT_CALL(*production_consensus, getValidatorStatus(_, _))
      .WillRepeatedly(Return(ValidatorStatus::NonValidator));
  EXPECT_CALL(*production_consensus, processSlot(_, best_block)).Times(0);

  timeline->start();
  EXPECT_FALSE(timeline->wasSynchronized());
  EXPECT_EQ(timeline->getCurrentState(), SyncState::WAIT_REMOTE_STATUS);
}

/**
 * @given start timeline
 * @when consensus returns we are single validator
 * @then we immediately have synchronized, and trying to process slot
 */
TEST_F(TimelineTest, SingleValidator) {
  EXPECT_CALL(clock, now()).Times(testing::AnyNumber());
  EXPECT_CALL(*slots_util, slotFinishTime(_)).Times(testing::AnyNumber());
  EXPECT_CALL(*block_tree, bestLeaf()).WillRepeatedly(Return(best_block));
  EXPECT_CALL(*production_consensus, getValidatorStatus(_, _))
      .WillRepeatedly(Return(ValidatorStatus::SingleValidator));
  EXPECT_CALL(*production_consensus, processSlot(_, best_block))
      .WillOnce(Return(outcome::success()));

  timeline->start();
  EXPECT_TRUE(timeline->wasSynchronized());
  EXPECT_EQ(timeline->getCurrentState(), SyncState::SYNCHRONIZED);
}

/**
 * @given start timeline
 * @when consensus returns we are single validator
 * @then we immediately have synchronized, and trying to process slot
 */
TEST_F(TimelineTest, Validator) {
  // launch

  EXPECT_CALL(clock, now()).Times(testing::AnyNumber());
  EXPECT_CALL(*slots_util, slotFinishTime(_)).Times(testing::AnyNumber());
  EXPECT_CALL(*block_tree, bestLeaf()).WillRepeatedly(Return(best_block));
  EXPECT_CALL(*production_consensus, getValidatorStatus(_, _))
      .WillRepeatedly(Return(ValidatorStatus::Validator));
  EXPECT_CALL(*production_consensus, processSlot(_, best_block)).Times(0);

  timeline->start();

  EXPECT_FALSE(timeline->wasSynchronized());
  EXPECT_EQ(timeline->getCurrentState(), SyncState::WAIT_REMOTE_STATUS);

  Mock::VerifyAndClearExpectations(production_consensus.get());

  // receive

  EXPECT_CALL(*block_tree, getBestContaining(_, _))
      .WillOnce(Return(best_block));
  EXPECT_CALL(*production_consensus, getSlot(best_block_header))
      .WillRepeatedly(Return(0));
  EXPECT_CALL(*production_consensus, processSlot(_, best_block))
      .WillOnce(Return(outcome::success()));

  timeline->onBlockAnnounceHandshake("peer"_peerid,
                                     {{}, best_block, best_block_hash});

  EXPECT_TRUE(timeline->wasSynchronized());
  EXPECT_EQ(timeline->getCurrentState(), SyncState::SYNCHRONIZED);
}

/**
 * @given start timeline
 * @when consensus returns we are single validator
 * @then we immediately have synchronized, and trying to process slot
 */
TEST_F(TimelineTest, Timing) {
  auto breaker = [] { throw std::logic_error("Must not be called"); };
  std::function<void()> on_run_slot_2 = breaker;
  std::function<void()> on_run_slot_3 = breaker;
  testing::Sequence s;
  EXPECT_CALL(*scheduler, scheduleImplMockCall(_, _, false))
      .InSequence(s)
      .WillOnce(WithArg<0>(Invoke([&](auto cb) {
        on_run_slot_2 = std::move(cb);
        return SchedulerMock::Handle{};
      })));

  // launch

  EXPECT_CALL(clock, now()).Times(testing::AnyNumber());
  EXPECT_CALL(*slots_util, slotFinishTime(_)).Times(testing::AnyNumber());
  EXPECT_CALL(*block_tree, bestLeaf()).WillRepeatedly(Return(best_block));
  EXPECT_CALL(*production_consensus, getValidatorStatus(_, _))
      .WillRepeatedly(Return(ValidatorStatus::Validator));
  EXPECT_CALL(*scheduler, scheduleImplMockCall(_, _, _)).Times(0);
  EXPECT_CALL(*production_consensus, processSlot(_, best_block)).Times(0);

  timeline->start();

  EXPECT_FALSE(timeline->wasSynchronized());
  EXPECT_EQ(timeline->getCurrentState(), SyncState::WAIT_REMOTE_STATUS);

  Mock::VerifyAndClearExpectations(production_consensus.get());

  // receive

  EXPECT_CALL(*block_tree, getBestContaining(_, _))
      .WillOnce(Return(best_block));
  EXPECT_CALL(*production_consensus, getSlot(best_block_header))
      .WillRepeatedly(Return(0));
  EXPECT_CALL(*production_consensus, processSlot(_, best_block))
      .WillOnce(Return(outcome::success()));

  timeline->onBlockAnnounceHandshake("peer"_peerid,
                                     {{}, best_block, best_block_hash});

  EXPECT_TRUE(timeline->wasSynchronized());
  EXPECT_EQ(timeline->getCurrentState(), SyncState::SYNCHRONIZED);
}
