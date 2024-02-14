/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/range/adaptor/transformed.hpp>

#include "common/main_thread_pool.hpp"
#include "common/worker_thread_pool.hpp"
#include "consensus/babe/impl/babe.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/types/babe_configuration.hpp"
#include "consensus/block_production_error.hpp"
#include "consensus/timeline/impl/slot_leadership_error.hpp"
#include "crypto/blake2/blake2b.h"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/authorship/proposer_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/clock/clock_mock.hpp"
#include "mock/core/consensus/babe/babe_block_validator_mock.hpp"
#include "mock/core/consensus/babe/babe_config_repository_mock.hpp"
#include "mock/core/consensus/babe/babe_lottery_mock.hpp"
#include "mock/core/consensus/timeline/slots_util_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/crypto/session_keys_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/crypto/vrf_provider_mock.hpp"
#include "mock/core/dispute_coordinator/dispute_coordinator_mock.hpp"
#include "mock/core/network/block_announce_transmitter_mock.hpp"
#include "mock/core/parachain/backing_store_mock.hpp"
#include "mock/core/parachain/bitfield_store_mock.hpp"
#include "mock/core/runtime/offchain_worker_api_mock.hpp"
#include "primitives/event_types.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"
#include "testutil/lazy.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/sr25519_utils.hpp"
#include "utils/watchdog.hpp"

using kagome::TestThreadPool;
using kagome::Watchdog;
using kagome::application::AppConfigurationMock;
using kagome::application::AppStateManagerMock;
using kagome::authorship::ProposerMock;
using kagome::blockchain::BlockTreeMock;
using kagome::clock::SystemClockMock;
using kagome::common::Buffer;
using kagome::common::BufferView;
using kagome::common::MainPoolHandler;
using kagome::common::MainThreadPool;
using kagome::common::uint256_to_le_bytes;
using kagome::common::WorkerPoolHandler;
using kagome::common::WorkerThreadPool;
using kagome::consensus::BlockProductionError;
using kagome::consensus::Duration;
using kagome::consensus::EpochLength;
using kagome::consensus::EpochNumber;
using kagome::consensus::EpochTimings;
using kagome::consensus::SlotLeadershipError;
using kagome::consensus::SlotNumber;
using kagome::consensus::SlotsUtil;
using kagome::consensus::SlotsUtilMock;
using kagome::consensus::Threshold;
using kagome::consensus::ValidatorStatus;
using kagome::consensus::babe::Authorities;
using kagome::consensus::babe::Authority;
using kagome::consensus::babe::AuthorityId;
using kagome::consensus::babe::AuthorityIndex;
using kagome::consensus::babe::Babe;
using kagome::consensus::babe::BabeBlockHeader;
using kagome::consensus::babe::BabeBlockValidator;
using kagome::consensus::babe::BabeBlockValidatorMock;
using kagome::consensus::babe::BabeConfigRepositoryMock;
using kagome::consensus::babe::BabeConfiguration;
using kagome::consensus::babe::BabeLotteryMock;
using kagome::consensus::babe::DigestError;
using kagome::consensus::babe::Randomness;
using kagome::consensus::babe::SlotLeadership;
using kagome::consensus::babe::SlotType;
using kagome::crypto::HasherMock;
using kagome::crypto::SessionKeysMock;
using kagome::crypto::Sr25519Keypair;
using kagome::crypto::Sr25519ProviderMock;
using kagome::crypto::Sr25519PublicKey;
using kagome::crypto::Sr25519Signature;
using kagome::crypto::VRFOutput;
using kagome::crypto::VRFPreOutput;
using kagome::crypto::VRFProof;
using kagome::crypto::VRFProviderMock;
using kagome::crypto::VRFVerifyOutput;
using kagome::dispute::DisputeCoordinatorMock;
using kagome::dispute::MultiDisputeStatementSet;
using kagome::network::BlockAnnounceTransmitterMock;
using kagome::parachain::BackingStoreMock;
using kagome::parachain::BitfieldStoreMock;
using kagome::primitives::Block;
using kagome::primitives::BlockBody;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockInfo;
using kagome::primitives::ConsensusEngineId;
using kagome::primitives::Digest;
using kagome::primitives::Extrinsic;
using kagome::primitives::PreRuntime;
using kagome::primitives::events::ChainSubscriptionEngine;
using kagome::primitives::events::StorageSubscriptionEngine;
using kagome::runtime::OffchainWorkerApiMock;
using kagome::storage::trie::calculateOrderedTrieHash;
using kagome::storage::trie::StateVersion;
using Seal = kagome::consensus::babe::Seal;
using SealDigest = kagome::primitives::Seal;

using testing::_;
using testing::Mock;
using testing::Return;

using namespace std::chrono_literals;

// TODO (kamilsa): workaround unless we bump gtest version to 1.8.1+
namespace kagome::primitives {
  std::ostream &operator<<(std::ostream &s,
                           const detail::DigestItemCommon &dic) {
    return s;
  }
}  // namespace kagome::primitives

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

class BabeTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    block_tree = std::make_shared<BlockTreeMock>();
    ON_CALL(*block_tree, getLastFinalized())
        .WillByDefault(Return(best_block_info));

    slots_util = std::make_shared<SlotsUtilMock>();

    // add initialization logic
    timings = {60ms, 2};
    our_keypair = std::make_shared<Sr25519Keypair>(generateSr25519Keypair());
    other_keypair = std::make_shared<Sr25519Keypair>(generateSr25519Keypair());
    babe_config = std::make_shared<BabeConfiguration>();
    babe_config->slot_duration = timings.slot_duration;
    babe_config->randomness.fill(0);
    babe_config->authorities = {Authority{{our_keypair->public_key}, 1},
                                Authority{{other_keypair->public_key}, 1}};
    babe_config->leadership_rate = {1, 4};
    babe_config->epoch_length = timings.epoch_length;

    babe_config_repo = std::make_shared<BabeConfigRepositoryMock>();
    ON_CALL(*babe_config_repo, config(_, _)).WillByDefault(Return(babe_config));

    session_keys = std::make_shared<SessionKeysMock>();
    ON_CALL(*session_keys, getBabeKeyPair(babe_config->authorities))
        .WillByDefault(Return(std::make_pair(our_keypair, 1)));

    lottery = std::make_shared<BabeLotteryMock>();

    hasher = std::make_shared<HasherMock>();
    static const auto d1 = scale::encode(genesis_block_header).value();
    ON_CALL(*hasher, blake2b_256(BufferView(d1)))
        .WillByDefault(Return(genesis_block_info.hash));
    static const auto d2 = scale::encode(best_block_header).value();
    ON_CALL(*hasher, blake2b_256(BufferView(d2)))
        .WillByDefault(Return(best_block_info.hash));
    static const auto d3 = scale::encode(new_block_header).value();
    ON_CALL(*hasher, blake2b_256(BufferView(d3)))
        .WillByDefault(Return(new_block_info.hash));

    sr25519_provider = std::make_shared<Sr25519ProviderMock>();
    block_validator = std::make_shared<BabeBlockValidatorMock>();
    bitfield_store = std::make_shared<BitfieldStoreMock>();
    backing_store = std::make_shared<BackingStoreMock>();

    dispute_coordinator = std::make_shared<DisputeCoordinatorMock>();
    ON_CALL(*dispute_coordinator, getDisputeForInherentData(_, _))
        .WillByDefault(testing::WithArg<1>(testing::Invoke(
            [](const auto &f) { f(MultiDisputeStatementSet{}); })));

    proposer = std::make_shared<ProposerMock>();

    storage_sub_engine = std::make_shared<StorageSubscriptionEngine>();
    chain_sub_engine = std::make_shared<ChainSubscriptionEngine>();
    announce_transmitter = std::make_shared<BlockAnnounceTransmitterMock>();

    offchain_worker_api = std::make_shared<OffchainWorkerApiMock>();
    ON_CALL(*offchain_worker_api, offchain_worker(_, _))
        .WillByDefault(Return(outcome::success()));

    watchdog = std::make_shared<Watchdog>(std::chrono::milliseconds(100));

    app_state_manager =
        std::make_shared<kagome::application::AppStateManagerMock>();

    main_thread_pool = std::make_shared<MainThreadPool>(
        watchdog, std::make_shared<boost::asio::io_context>());
    main_pool_handler =
        std::make_shared<MainPoolHandler>(app_state_manager, main_thread_pool);
    main_pool_handler->start();

    worker_thread_pool = std::make_shared<WorkerThreadPool>(watchdog, 1);
    worker_pool_handler = std::make_shared<WorkerPoolHandler>(
        app_state_manager, worker_thread_pool);
    worker_pool_handler->start();

    babe = std::make_shared<Babe>(app_config,
                                  clock,
                                  block_tree,
                                  testutil::sptr_to_lazy<SlotsUtil>(slots_util),
                                  babe_config_repo,
                                  timings,
                                  session_keys,
                                  lottery,
                                  hasher,
                                  sr25519_provider,
                                  block_validator,
                                  bitfield_store,
                                  backing_store,
                                  dispute_coordinator,
                                  proposer,
                                  storage_sub_engine,
                                  chain_sub_engine,
                                  announce_transmitter,
                                  offchain_worker_api,
                                  // main_pool_handler,
                                  // worker_pool_handler);
                                  TestThreadPool{io_},
                                  io_);
  }

  void TearDown() override {
    watchdog->stop();
    worker_thread_pool.reset();
    main_pool_handler.reset();
  }

  AppConfigurationMock app_config;
  SystemClockMock clock;
  std::shared_ptr<BlockTreeMock> block_tree;
  std::shared_ptr<SlotsUtilMock> slots_util;
  std::shared_ptr<BabeConfigRepositoryMock> babe_config_repo;
  EpochTimings timings;
  std::shared_ptr<SessionKeysMock> session_keys;
  std::shared_ptr<BabeLotteryMock> lottery;
  std::shared_ptr<HasherMock> hasher;
  std::shared_ptr<Sr25519ProviderMock> sr25519_provider;
  std::shared_ptr<BabeBlockValidatorMock> block_validator;
  std::shared_ptr<BitfieldStoreMock> bitfield_store;
  std::shared_ptr<BackingStoreMock> backing_store;
  std::shared_ptr<DisputeCoordinatorMock> dispute_coordinator;
  std::shared_ptr<ProposerMock> proposer;
  std::shared_ptr<StorageSubscriptionEngine> storage_sub_engine;
  std::shared_ptr<ChainSubscriptionEngine> chain_sub_engine;
  std::shared_ptr<BlockAnnounceTransmitterMock> announce_transmitter;
  std::shared_ptr<OffchainWorkerApiMock> offchain_worker_api;
  std::shared_ptr<boost::asio::io_context> io_ =
      std::make_shared<boost::asio::io_context>();
  std::shared_ptr<AppStateManagerMock> app_state_manager;
  std::shared_ptr<Watchdog> watchdog;
  std::shared_ptr<MainThreadPool> main_thread_pool;
  std::shared_ptr<MainPoolHandler> main_pool_handler;
  std::shared_ptr<WorkerThreadPool> worker_thread_pool;
  std::shared_ptr<WorkerPoolHandler> worker_pool_handler;

  std::shared_ptr<BabeConfiguration> babe_config;

  std::shared_ptr<Sr25519Keypair> our_keypair;
  std::shared_ptr<Sr25519Keypair> other_keypair;

  static constexpr EpochNumber uninitialized_epoch =
      std::numeric_limits<EpochNumber>::max();

  std::shared_ptr<Babe> babe;  // testee

  const BlockInfo genesis_block_info{0, "block#0"_hash256};
  const BlockHeader genesis_block_header{
      genesis_block_info.number,   // number
      {},                          // parent
      "state_root#0"_hash256,      // state_root
      "extrinsic_root#0"_hash256,  // extrinsic_root
      make_digest(10000),          // digest
      genesis_block_info.hash      // hash
  };

  const BlockInfo best_block_info{100, "block#100"_hash256};
  const SlotNumber best_block_slot = 1000;
  const BlockHeader best_block_header{
      best_block_info.number,        // number
      "block#99"_hash256,            // parent
      "state_root#100"_hash256,      // state_root
      "extrinsic_root#100"_hash256,  // extrinsic_root
      make_digest(best_block_slot),  // digest
      best_block_info.hash           // hash
  };

  const BlockInfo new_block_info{best_block_info.number + 1,
                                 "block#101"_hash256};
  const SlotNumber new_block_slot = 1001;
  const Block new_block = [&] {
    Block block;
    block.body = {{{1}}, {{2}}, {{3}}};
    block.header = BlockHeader{
        new_block_info.number,     // number
        best_block_info.hash,      // parent
        "state_root#101"_hash256,  // state_root
        [&] {                      // extrinsic_root
          using boost::adaptors::transformed;
          return calculateOrderedTrieHash(
                     StateVersion::V0,
                     block.body | transformed([](const auto &ext) {
                       return Buffer{scale::encode(ext).value()};
                     }),
                     kagome::crypto::blake2b<32>)
              .value();
        }(),
        make_digest(new_block_slot),  // digest
        new_block_info.hash           // hash
    };
    return block;
  }();
  const std::vector<Extrinsic> &new_block_extrinsics = new_block.body;
  const BlockHeader &new_block_header = new_block.header;
};

TEST_F(BabeTest, Setup) {
  ASSERT_OUTCOME_ERROR(babe->getSlot(genesis_block_header),
                       DigestError::GENESIS_BLOCK_CAN_NOT_HAVE_DIGESTS);

  ASSERT_OUTCOME_SUCCESS(actual_slot, babe->getSlot(best_block_header));
  EXPECT_EQ(actual_slot, best_block_slot);

  EXPECT_EQ(babe->getValidatorStatus(best_block_info, 0),
            ValidatorStatus::Validator);
}

TEST_F(BabeTest, NonValidator) {
  SlotNumber slot = new_block_slot;
  EpochNumber epoch = 0;

  babe_config->authorities = {Authority{{other_keypair->public_key}, 1}};

  EXPECT_CALL(*slots_util, timeToSlot(_)).WillOnce(Return(slot));
  EXPECT_CALL(*slots_util, slotToEpoch(best_block_info, slot))
      .WillOnce(Return(outcome::success(epoch)));

  EXPECT_CALL(*lottery, getEpoch())
      .WillOnce(Return(uninitialized_epoch))
      .WillRepeatedly(Return(epoch));
  EXPECT_CALL(*lottery, changeEpoch(epoch, best_block_info))
      .WillOnce(Return(false));

  EXPECT_EQ(babe->getValidatorStatus(best_block_info, slot),
            ValidatorStatus::NonValidator);

  ASSERT_OUTCOME_ERROR(babe->processSlot(slot, best_block_info),
                       SlotLeadershipError::NO_VALIDATOR);
}

TEST_F(BabeTest, NoSlotLeader) {
  SlotNumber slot = new_block_slot;
  EpochNumber epoch = 0;

  EXPECT_CALL(*slots_util, timeToSlot(_)).WillOnce(Return(slot));
  EXPECT_CALL(*slots_util, slotToEpoch(best_block_info, slot))
      .WillOnce(Return(outcome::success(epoch)));

  EXPECT_CALL(*lottery, getEpoch())
      .WillOnce(Return(uninitialized_epoch))
      .WillRepeatedly(Return(epoch));
  EXPECT_CALL(*lottery, changeEpoch(epoch, best_block_info))
      .WillOnce(Return(true));
  EXPECT_CALL(*lottery, getSlotLeadership(best_block_info.hash, slot))
      .WillOnce(Return(std::nullopt));

  EXPECT_EQ(babe->getValidatorStatus(best_block_info, slot),
            ValidatorStatus::Validator);

  ASSERT_OUTCOME_ERROR(babe->processSlot(slot, best_block_info),
                       SlotLeadershipError::NO_SLOT_LEADER);
}

TEST_F(BabeTest, SlotLeader) {
  SlotNumber slot = new_block_slot;
  EpochNumber epoch = 0;

  EXPECT_CALL(*slots_util, timeToSlot(_)).WillOnce(Return(slot));
  EXPECT_CALL(*slots_util, slotToEpoch(best_block_info, slot))
      .WillOnce(Return(outcome::success(epoch)));

  EXPECT_CALL(*lottery, getEpoch())
      .WillOnce(Return(uninitialized_epoch))
      .WillRepeatedly(Return(epoch));
  EXPECT_CALL(*lottery, changeEpoch(epoch, best_block_info))
      .WillOnce(Return(true));
  EXPECT_CALL(*lottery, getSlotLeadership(best_block_info.hash, slot))
      .WillOnce(Return(SlotLeadership{.keypair = our_keypair}));

  EXPECT_CALL(*block_tree, getBlockHeader(best_block_info.hash))
      .WillOnce(Return(best_block_header));

  EXPECT_CALL(*proposer, propose(best_block_info, _, _, _, _))
      .WillOnce(Return(outcome::success(new_block)));

  EXPECT_CALL(*sr25519_provider, sign(*our_keypair, _))
      .WillOnce(Return(outcome::success(Sr25519Signature{})));

  EXPECT_CALL(*block_tree, addBlock(_)).WillOnce(Return(outcome::success()));

  EXPECT_EQ(babe->getValidatorStatus(best_block_info, slot),
            ValidatorStatus::Validator);

  ASSERT_OUTCOME_SUCCESS_TRY(babe->processSlot(slot, best_block_info));

  io_->run();
}
