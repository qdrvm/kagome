/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <mock/libp2p/basic/scheduler_mock.hpp>

#include "common/main_thread_pool.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "consensus/babe/types/seal.hpp"
#include "consensus/babe/types/slot_type.hpp"
#include "consensus/timeline/impl/slot_leadership_error.hpp"
#include "consensus/timeline/impl/timeline_impl.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/clock/clock_mock.hpp"
#include "mock/core/consensus/grandpa/grandpa_mock.hpp"
#include "mock/core/consensus/production_consensus_mock.hpp"
#include "mock/core/consensus/timeline/consensus_selector_mock.hpp"
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

using kagome::TestThreadPool;
using kagome::application::AppConfigurationMock;
using kagome::application::AppStateManagerMock;
using kagome::blockchain::BlockTreeMock;
using kagome::clock::SystemClockMock;
using kagome::common::Buffer;
using kagome::common::MainThreadPool;
using kagome::consensus::ConsensusSelectorMock;
using kagome::consensus::Duration;
using kagome::consensus::EpochLength;
using kagome::consensus::EpochNumber;
using kagome::consensus::EpochTimings;
using kagome::consensus::ProductionConsensusMock;
using kagome::consensus::SlotLeadershipError;
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
using kagome::primitives::events::ChainSubscriptionEngine;
using kagome::primitives::events::SyncStateSubscriptionEngine;
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
}

class TimelineTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers(soralog::Level::DEBUG);
  }

  void SetUp() override {
    app_state_manager = std::make_shared<AppStateManagerMock>();

    EpochTimings timings = {
        .slot_duration = 6s,
        .epoch_length = 200,
    };

    slots_util = std::make_shared<SlotsUtilMock>();
    ON_CALL(*slots_util, slotDuration())
        .WillByDefault(Return(timings.slot_duration));
    ON_CALL(*slots_util, epochLength())
        .WillByDefault(Return(timings.epoch_length));
    ON_CALL(*slots_util, timeToSlot(_)).WillByDefault(Invoke([&] {
      return current_slot;
    }));
    ON_CALL(*slots_util, slotToEpoch(_, _))
        .WillByDefault(WithArg<1>(Invoke([timings](auto slot) -> EpochNumber {
          return slot / timings.epoch_length;
        })));

    block_tree = std::make_shared<BlockTreeMock>();
    ON_CALL(*block_tree, bestBlock()).WillByDefault(Return(best_block));
    ON_CALL(*block_tree, getLastFinalized()).WillByDefault(Return(best_block));
    ON_CALL(*block_tree, getBlockHeader(best_block.hash))
        .WillByDefault(Return(best_block_header));

    consensus_selector = std::make_shared<ConsensusSelectorMock>();
    production_consensus = std::make_shared<ProductionConsensusMock>();
    ON_CALL(*consensus_selector, getProductionConsensusByInfo(_))
        .WillByDefault(Return(production_consensus));
    ON_CALL(*consensus_selector, getProductionConsensusByHeader(_))
        .WillByDefault(Return(production_consensus));
    ON_CALL(*production_consensus, getSlot(best_block_header))
        .WillByDefault(Return(1));

    trie_storage = std::make_shared<TrieStorageMock>();
    synchronizer = std::make_shared<SynchronizerMock>();
    hasher = std::make_shared<HasherMock>();
    block_announce_transmitter =
        std::make_shared<BlockAnnounceTransmitterMock>();
    // warp_sync = std::make_shared<WarpSync>();
    // warp_protocol = std::make_shared<WarpProtocol>();
    justification_observer = std::make_shared<GrandpaMock>();
    scheduler = std::make_shared<SchedulerMock>();
    core_api = std::make_shared<CoreMock>();
    chain_sub_engine = std::make_shared<ChainSubscriptionEngine>();
    state_sub_engine = std::make_shared<SyncStateSubscriptionEngine>();

    MainThreadPool main_thread{TestThreadPool{io}};
    timeline = std::make_shared<TimelineImpl>(
        app_config,
        app_state_manager,
        clock,
        main_thread,
        slots_util,
        block_tree,
        consensus_selector,
        trie_storage,
        nullptr,
        synchronizer,
        hasher,
        block_announce_transmitter,
        warp_sync,
        testutil::sptr_to_lazy<WarpProtocol>(warp_protocol),
        justification_observer,
        scheduler,
        chain_sub_engine,
        state_sub_engine,
        core_api);
  }

  std::shared_ptr<boost::asio::io_context> io =
      std::make_shared<boost::asio::io_context>();
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
  std::shared_ptr<SchedulerMock> scheduler;
  std::shared_ptr<ChainSubscriptionEngine> chain_sub_engine;
  std::shared_ptr<SyncStateSubscriptionEngine> state_sub_engine;
  std::shared_ptr<CoreMock> core_api;

  std::shared_ptr<TimelineImpl> timeline;

  std::shared_ptr<ProductionConsensusMock> production_consensus;

  SlotNumber current_slot;

  BlockInfo genesis_block{0, "block#0"_hash256};
  BlockHeader genesis_block_header{
      genesis_block.number,  // number
      {},                    // parent
      {},                    // state_root
      {},                    // extrinsic_root
      {}                     // digest
  };

  BlockInfo best_block{1, "block#1"_hash256};
  SlotNumber best_block_slot = 1;
  BlockHeader best_block_header{
      best_block.number,            // number
      genesis_block.hash,           // parent
      {},                           // state_root
      {},                           // extrinsic_root
      make_digest(best_block_slot)  // digest
  };
};

/**
 * @given start timeline
 * @when consensus returns we are not validator
 * @then we has not synchronized and waiting announce or incoming stream
 */
TEST_F(TimelineTest, NonValidator) {
  EXPECT_CALL(clock, now());
  EXPECT_CALL(*slots_util, slotFinishTime(_)).Times(testing::AnyNumber());
  EXPECT_CALL(*production_consensus, getValidatorStatus(_, _))
      .WillRepeatedly(Return(ValidatorStatus::NonValidator));
  EXPECT_CALL(*production_consensus, processSlot(_, best_block)).Times(0);

  timeline->start();
  EXPECT_FALSE(timeline->wasSynchronized());
  EXPECT_EQ(timeline->getCurrentState(), SyncState::WAIT_REMOTE_STATUS);
}

/**
 * @given start timeline in slot 2. best block was produced in slot 1
 * @when consensus returns we are single validator
 * @then we immediately have synchronized, and trying to process slot
 */
TEST_F(TimelineTest, SingleValidator) {
  auto breaker = [] { throw std::logic_error("Must not be called"); };
  std::function<void()> on_run_slot = breaker;

  // LAUNCH (best block on slot 1)
  {
    current_slot = 1;
    EXPECT_CALL(*production_consensus, getValidatorStatus(_, _))
        .WillRepeatedly(Return(ValidatorStatus::SingleValidator));
    EXPECT_CALL(*production_consensus, processSlot(_, best_block)).Times(0);
    //  - start to wait for end of current slot
    EXPECT_CALL(*scheduler, scheduleImpl(_, _, false))
        .WillOnce(WithArg<0>(Invoke([&](auto cb) {
          on_run_slot = std::move(cb);
          return SchedulerMock::Handle{};
        })));

    timeline->start();

    io->run_one();

    EXPECT_TRUE(timeline->wasSynchronized());
    EXPECT_EQ(timeline->getCurrentState(), SyncState::SYNCHRONIZED);

    Mock::VerifyAndClearExpectations(production_consensus.get());
    Mock::VerifyAndClearExpectations(scheduler.get());
  }

  // SLOT 2
  {
    ++current_slot;
    ASSERT_EQ(current_slot, 2);

    // when: timer goes off
    // then:
    //  - process slot (successful for this case)
    EXPECT_CALL(*production_consensus, processSlot(current_slot, best_block))
        .WillOnce(Return(outcome::success()));
    //  - start to wait for end of current slot
    EXPECT_CALL(*scheduler, scheduleImpl(_, _, false))
        .WillOnce(WithArg<0>(
            Invoke([&](auto cb) { return SchedulerMock::Handle{}; })));

    on_run_slot();

    // - node continues to be synchronized
    EXPECT_EQ(timeline->getCurrentState(), SyncState::SYNCHRONIZED);

    Mock::VerifyAndClearExpectations(production_consensus.get());
    Mock::VerifyAndClearExpectations(scheduler.get());
  }
}

/**
 * @given start timeline
 * @when consensus returns we are single validator
 * @then we immediately have synchronized, and trying to process slot
 */
TEST_F(TimelineTest, Validator) {
  auto breaker = [] { throw std::logic_error("Must not be called"); };
  std::function<void()> on_run_slot_2 = breaker;
  std::function<void()> on_run_slot_3 = breaker;

  // given: just created Timeline

  // LAUNCH
  {
    current_slot = 1;

    // when: call start
    // then:
    //  - get validator status to know if needed to participate in block
    //    production
    EXPECT_CALL(*production_consensus, getValidatorStatus(_, _))
        .WillRepeatedly(Return(ValidatorStatus::Validator));
    //  - don't process slot, because node is not synchronized
    EXPECT_CALL(*production_consensus, processSlot(_, best_block)).Times(0);
    //  - don't wait time to run slot, because node is not synchronized
    EXPECT_CALL(*scheduler, scheduleImpl(_, _, _)).Times(0);

    timeline->start();

    //  - node isn't synchronized
    EXPECT_FALSE(timeline->wasSynchronized());
    //  - node is waiting data from remove peers
    EXPECT_EQ(timeline->getCurrentState(), SyncState::WAIT_REMOTE_STATUS);

    Mock::VerifyAndClearExpectations(production_consensus.get());
    Mock::VerifyAndClearExpectations(scheduler.get());
  }

  // SYNC (will be finished on slot 1)
  {
    ASSERT_EQ(current_slot, 1);

    // when: receive remote peer data enough to become synchronized
    // then:
    //  - check by slot if caught up after loading blocks
    EXPECT_CALL(*production_consensus, getSlot(best_block_header))
        .WillRepeatedly(Return(0));
    //  - process slot won't start, because slot is not changed
    EXPECT_CALL(*production_consensus, processSlot(_, _)).Times(0);
    //  - start to wait for end of current slot
    EXPECT_CALL(*scheduler, scheduleImpl(_, _, false))
        .WillOnce(WithArg<0>(Invoke([&](auto cb) {
          on_run_slot_2 = std::move(cb);
          return SchedulerMock::Handle{};
        })));

    timeline->onBlockAnnounceHandshake("peer"_peerid,
                                       {{}, best_block, best_block.hash});

    // - node is synchronized now
    EXPECT_TRUE(timeline->wasSynchronized());
    EXPECT_EQ(timeline->getCurrentState(), SyncState::SYNCHRONIZED);

    Mock::VerifyAndClearExpectations(production_consensus.get());
    Mock::VerifyAndClearExpectations(scheduler.get());
  }

  // SLOT 2 (nobody will add new block for this case)
  {
    ++current_slot;
    ASSERT_EQ(current_slot, 2);

    // when: receive remote peer data enough to become synchronized
    // then:
    //  - check by slot if caught up after loading blocks
    EXPECT_CALL(*production_consensus, getSlot(best_block_header))
        .WillRepeatedly(Return(0));
    //  - process slot (not slot leader for this case)
    EXPECT_CALL(*production_consensus, processSlot(current_slot, best_block))
        .WillOnce(Return(SlotLeadershipError::NO_SLOT_LEADER));
    //  - start to wait for end of current slot
    EXPECT_CALL(*scheduler, scheduleImpl(_, _, false))
        .WillOnce(WithArg<0>(Invoke([&](auto cb) {
          on_run_slot_3 = std::move(cb);
          return SchedulerMock::Handle{};
        })));

    on_run_slot_2();

    // - node is synchronized now
    EXPECT_EQ(timeline->getCurrentState(), SyncState::SYNCHRONIZED);

    Mock::VerifyAndClearExpectations(production_consensus.get());
    Mock::VerifyAndClearExpectations(scheduler.get());
  }

  // SLOT 3
  {
    ++current_slot;
    ASSERT_EQ(current_slot, 3);

    // when: timer goes off
    // then:
    //  - process slot (successful for this case)
    EXPECT_CALL(*production_consensus, processSlot(current_slot, best_block))
        .WillOnce(Return(outcome::success()));
    //  - start to wait for end of current slot
    EXPECT_CALL(*scheduler, scheduleImpl(_, _, false))
        .WillOnce(WithArg<0>(Invoke([&](auto cb) {
          on_run_slot_3 = std::move(cb);
          return SchedulerMock::Handle{};
        })));

    on_run_slot_3();

    // - node continues to be synchronized
    EXPECT_EQ(timeline->getCurrentState(), SyncState::SYNCHRONIZED);

    Mock::VerifyAndClearExpectations(production_consensus.get());
    Mock::VerifyAndClearExpectations(scheduler.get());
  }
}

/**
 * @given start timeline
 * @when consensus returns we are not validator
 * @then we has not synchronized and waiting announce or incoming stream
 */
TEST_F(TimelineTest, Equivocation) {
  BlockHeader new_block{
      10,                         // number
      "block_#9"_hash256,         // parent
      {},                         // state_root
      {},                         // extrinsic_root
      make_digest(10),            // digest
      "block_#10_s10_a0"_hash256  // hash
  };

  EXPECT_CALL(*production_consensus, getSlot(new_block)).WillOnce(Return(10));
  EXPECT_CALL(*production_consensus, getAuthority(new_block))
      .WillOnce(Return(0));
  EXPECT_CALL(*production_consensus, reportEquivocation(_, _)).Times(0);
  timeline->checkAndReportEquivocation(new_block);

  BlockHeader another_slot_block{
      10,                         // number
      "block_#9_fork"_hash256,    // parent
      {},                         // state_root
      {},                         // extrinsic_root
      make_digest(11),            // digest
      "block_#10_s11_a0"_hash256  // hash
  };

  EXPECT_CALL(*production_consensus, getSlot(another_slot_block))
      .WillOnce(Return(11));
  EXPECT_CALL(*production_consensus, getAuthority(another_slot_block))
      .WillOnce(Return(0));
  EXPECT_CALL(*production_consensus, reportEquivocation(_, _)).Times(0);
  timeline->checkAndReportEquivocation(another_slot_block);

  BlockHeader another_validator_block{
      10,                         // number
      "block_#9"_hash256,         // parent
      {},                         // state_root
      {},                         // extrinsic_root
      make_digest(10),            // digest
      "block_#10_s10_a1"_hash256  // hash
  };

  EXPECT_CALL(*production_consensus, getSlot(another_validator_block))
      .WillOnce(Return(10));
  EXPECT_CALL(*production_consensus, getAuthority(another_validator_block))
      .WillOnce(Return(1));
  EXPECT_CALL(*production_consensus, reportEquivocation(_, _)).Times(0);
  timeline->checkAndReportEquivocation(another_validator_block);

  BlockHeader equivocating_block{
      10,                            // number
      "block_#9"_hash256,            // parent
      {},                            // state_root
      {},                            // extrinsic_root
      make_digest(10),               // digest
      "block_#10_s10_a0_e1"_hash256  // hash
  };

  EXPECT_CALL(*production_consensus, getSlot(equivocating_block))
      .WillOnce(Return(10));
  EXPECT_CALL(*production_consensus, getAuthority(equivocating_block))
      .WillOnce(Return(0));
  EXPECT_CALL(*production_consensus,
              reportEquivocation(new_block.hash(), equivocating_block.hash()))
      .WillOnce(Return(outcome::success()));
  timeline->checkAndReportEquivocation(equivocating_block);

  BlockHeader one_more_equivocating_block{
      10,                            // number
      "block_#9"_hash256,            // parent
      {},                            // state_root
      {},                            // extrinsic_root
      make_digest(10),               // digest
      "block_#10_s10_a0_e2"_hash256  // hash
  };

  EXPECT_CALL(*production_consensus, getSlot(one_more_equivocating_block))
      .WillOnce(Return(10));
  EXPECT_CALL(*production_consensus, getAuthority(one_more_equivocating_block))
      .WillOnce(Return(0));
  EXPECT_CALL(*production_consensus, reportEquivocation(_, _)).Times(0);
  timeline->checkAndReportEquivocation(one_more_equivocating_block);
}
