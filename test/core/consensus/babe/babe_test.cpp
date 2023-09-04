/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/range/adaptor/transformed.hpp>

#include "consensus/babe/impl/babe.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/babe_error.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/authorship/proposer_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/blockchain/digest_tracker_mock.hpp"
#include "mock/core/clock/clock_mock.hpp"
#include "mock/core/consensus/babe/babe_config_repository_mock.hpp"
#include "mock/core/consensus/babe_lottery_mock.hpp"
#include "mock/core/consensus/timeline/slots_util_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/crypto/session_keys_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/dispute_coordinator/dispute_coordinator_mock.hpp"
#include "mock/core/network/block_announce_transmitter_mock.hpp"
#include "mock/core/parachain/backing_store_mock.hpp"
#include "mock/core/parachain/bitfield_store_mock.hpp"
#include "mock/core/runtime/offchain_worker_api_mock.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/event_types.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"
#include "testutil/lazy.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/sr25519_utils.hpp"

using kagome::application::AppConfigurationMock;
using kagome::authorship::ProposerMock;
using kagome::blockchain::BlockTreeMock;
using kagome::blockchain::DigestTrackerMock;
using kagome::clock::SystemClockMock;
using kagome::common::Buffer;
using kagome::consensus::EpochLength;
using kagome::consensus::EpochNumber;
using kagome::consensus::SlotNumber;
using kagome::consensus::SlotsUtil;
using kagome::consensus::SlotsUtilMock;
using kagome::consensus::ValidatorStatus;
using kagome::consensus::babe::Babe;
using kagome::consensus::babe::BabeBlockHeader;
using kagome::consensus::babe::BabeConfigRepositoryMock;
using kagome::consensus::babe::BabeError;
using kagome::consensus::babe::BabeLotteryMock;
using kagome::consensus::babe::DigestError;
using kagome::consensus::babe::SlotType;
using kagome::crypto::HasherMock;
using kagome::crypto::SessionKeysMock;
using kagome::crypto::Sr25519Keypair;
using kagome::crypto::Sr25519ProviderMock;
using kagome::crypto::Sr25519Signature;
using kagome::crypto::VRFOutput;
using kagome::dispute::DisputeCoordinatorMock;
using kagome::dispute::MultiDisputeStatementSet;
using kagome::network::BlockAnnounceTransmitterMock;
using kagome::parachain::BackingStoreMock;
using kagome::parachain::BitfieldStoreMock;
using kagome::primitives::Authority;
using kagome::primitives::BabeConfiguration;
using kagome::primitives::Block;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockInfo;
using kagome::primitives::Digest;
using kagome::primitives::Duration;
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
    slot_duration = 60ms;
    epoch_length = 2;
    our_keypair = std::make_shared<Sr25519Keypair>(generateSr25519Keypair());
    other_keypair = std::make_shared<Sr25519Keypair>(generateSr25519Keypair());
    babe_config = std::make_shared<BabeConfiguration>();
    babe_config->slot_duration = slot_duration;
    babe_config->randomness.fill(0);
    babe_config->authorities = {Authority{{our_keypair->public_key}, 1},
                                Authority{{other_keypair->public_key}, 1}};
    babe_config->leadership_rate = {1, 4};
    babe_config->epoch_length = epoch_length;

    babe_config_repo = std::make_shared<BabeConfigRepositoryMock>();
    ON_CALL(*babe_config_repo, config(_, _)).WillByDefault(Return(babe_config));
    ON_CALL(*babe_config_repo, slotDuration())
        .WillByDefault(Return(babe_config->slot_duration));
    ON_CALL(*babe_config_repo, epochLength())
        .WillByDefault(Return(babe_config->epoch_length));

    session_keys = std::make_shared<SessionKeysMock>();
    ON_CALL(*session_keys, getBabeKeyPair(babe_config->authorities))
        .WillByDefault(Return(std::make_pair(our_keypair, 0)));

    lottery = std::make_shared<BabeLotteryMock>();

    hasher = std::make_shared<HasherMock>();
    auto d1 = gsl::make_span(scale::encode(genesis_block_header).value());
    ON_CALL(*hasher, blake2b_256(d1))
        .WillByDefault(Return(genesis_block_info.hash));
    auto d2 = gsl::make_span(scale::encode(best_block_header).value());
    ON_CALL(*hasher, blake2b_256(d2))
        .WillByDefault(Return(best_block_info.hash));
    auto d3 = gsl::make_span(scale::encode(new_block_header).value());
    ON_CALL(*hasher, blake2b_256(d3))
        .WillByDefault(Return(new_block_info.hash));

    sr25519_provider = std::make_shared<Sr25519ProviderMock>();
    bitfield_store = std::make_shared<BitfieldStoreMock>();
    backing_store = std::make_shared<BackingStoreMock>();

    dispute_coordinator = std::make_shared<DisputeCoordinatorMock>();
    ON_CALL(*dispute_coordinator, getDisputeForInherentData(_, _))
        .WillByDefault(testing::WithArg<1>(testing::Invoke(
            [](const auto &f) { f(MultiDisputeStatementSet{}); })));

    proposer = std::make_shared<ProposerMock>();

    digest_tracker = std::make_shared<DigestTrackerMock>();
    ON_CALL(*digest_tracker, onDigest(_, _))
        .WillByDefault(Return(outcome::success()));

    storage_sub_engine = std::make_shared<StorageSubscriptionEngine>();
    chain_sub_engine = std::make_shared<ChainSubscriptionEngine>();
    announce_transmitter = std::make_shared<BlockAnnounceTransmitterMock>();

    offchain_worker_api = std::make_shared<OffchainWorkerApiMock>();
    ON_CALL(*offchain_worker_api, offchain_worker(_, _))
        .WillByDefault(Return(outcome::success()));

    babe = std::make_shared<Babe>(app_config,
                                  clock,
                                  block_tree,
                                  testutil::sptr_to_lazy<SlotsUtil>(slots_util),
                                  babe_config_repo,
                                  session_keys,
                                  lottery,
                                  hasher,
                                  sr25519_provider,
                                  bitfield_store,
                                  backing_store,
                                  dispute_coordinator,
                                  proposer,
                                  digest_tracker,
                                  storage_sub_engine,
                                  chain_sub_engine,
                                  announce_transmitter,
                                  offchain_worker_api);
  }

  AppConfigurationMock app_config;
  SystemClockMock clock;
  std::shared_ptr<BlockTreeMock> block_tree;
  std::shared_ptr<SlotsUtilMock> slots_util;
  std::shared_ptr<BabeConfigRepositoryMock> babe_config_repo;
  std::shared_ptr<SessionKeysMock> session_keys;
  std::shared_ptr<BabeLotteryMock> lottery;
  std::shared_ptr<HasherMock> hasher;
  std::shared_ptr<Sr25519ProviderMock> sr25519_provider;
  std::shared_ptr<BitfieldStoreMock> bitfield_store;
  std::shared_ptr<BackingStoreMock> backing_store;
  std::shared_ptr<DisputeCoordinatorMock> dispute_coordinator;
  std::shared_ptr<ProposerMock> proposer;
  std::shared_ptr<DigestTrackerMock> digest_tracker;
  std::shared_ptr<StorageSubscriptionEngine> storage_sub_engine;
  std::shared_ptr<ChainSubscriptionEngine> chain_sub_engine;
  std::shared_ptr<BlockAnnounceTransmitterMock> announce_transmitter;
  std::shared_ptr<OffchainWorkerApiMock> offchain_worker_api;

  Duration slot_duration = 3s;
  EpochLength epoch_length = 20;
  std::shared_ptr<BabeConfiguration> babe_config;

  std::shared_ptr<Sr25519Keypair> our_keypair;
  std::shared_ptr<Sr25519Keypair> other_keypair;

  std::shared_ptr<Babe> babe;  // testee

  const BlockInfo genesis_block_info{0, "block#0"_hash256};
  const BlockHeader genesis_block_header{
      genesis_block_info.number,   // number
      {},                          // parent
      "state_root#0"_hash256,      // state_root
      "extrinsic_root#0"_hash256,  // extrinsic_root
      make_digest(10000)           // digest
  };

  const BlockInfo best_block_info{100, "block#100"_hash256};
  const SlotNumber best_block_slot = 1000;
  const BlockHeader best_block_header{
      best_block_info.number,        // number
      "block#99"_hash256,            // parent
      "state_root#100"_hash256,      // state_root
      "extrinsic_root#100"_hash256,  // extrinsic_root
      make_digest(best_block_slot)   // digest
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
                     }))
              .value();
        }(),
        make_digest(new_block_slot)  // digest
    };
    return block;
  }();
  const std::vector<Extrinsic> &new_block_extrinsics = new_block.body;
  const BlockHeader &new_block_header = new_block.header;
};

TEST_F(BabeTest, Setup) {
  auto [actual_slot_duration, actual_epoch_length] = babe->getTimings();
  EXPECT_EQ(actual_slot_duration, slot_duration);
  EXPECT_EQ(actual_epoch_length, epoch_length);

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

  EXPECT_EQ(babe->getValidatorStatus(best_block_info, slot),
            ValidatorStatus::NonValidator);

  EXPECT_CALL(*slots_util, timeToSlot(_)).WillOnce(Return(slot));
  EXPECT_CALL(*slots_util, slotToEpoch(best_block_info, slot))
      .WillOnce(Return(outcome::success(epoch)));

  ASSERT_OUTCOME_ERROR(babe->processSlot(slot, best_block_info),
                       BabeError::NO_VALIDATOR);
}

TEST_F(BabeTest, NoSlotLeader) {
  SlotNumber slot = new_block_slot;
  EpochNumber epoch = 0;

  EXPECT_EQ(babe->getValidatorStatus(best_block_info, slot),
            ValidatorStatus::Validator);

  EXPECT_CALL(*slots_util, timeToSlot(_)).WillOnce(Return(slot));
  EXPECT_CALL(*slots_util, slotToEpoch(best_block_info, slot))
      .WillOnce(Return(outcome::success(epoch)));

  EXPECT_CALL(*lottery, getEpoch()).WillOnce(Return(epoch));
  EXPECT_CALL(*lottery, getSlotLeadership(slot)).WillOnce(Return(std::nullopt));

  ASSERT_OUTCOME_ERROR(babe->processSlot(slot, best_block_info),
                       BabeError::NO_SLOT_LEADER);
}

TEST_F(BabeTest, SlotLeader) {
  SlotNumber slot = new_block_slot;
  EpochNumber epoch = 0;

  EXPECT_EQ(babe->getValidatorStatus(best_block_info, slot),
            ValidatorStatus::Validator);

  EXPECT_CALL(*slots_util, timeToSlot(_)).WillOnce(Return(slot));
  EXPECT_CALL(*slots_util, slotToEpoch(best_block_info, slot))
      .WillOnce(Return(outcome::success(epoch)));

  EXPECT_CALL(*lottery, getEpoch()).WillOnce(Return(epoch));
  EXPECT_CALL(*lottery, getSlotLeadership(slot)).WillOnce(Return(VRFOutput{}));

  EXPECT_CALL(*block_tree, getBlockHeader(best_block_info.hash))
      .WillOnce(Return(best_block_header));

  EXPECT_CALL(*proposer, propose(best_block_info, _, _, _, _))
      .WillOnce(Return(outcome::success(new_block)));

  EXPECT_CALL(*sr25519_provider, sign(*our_keypair, _))
      .WillOnce(Return(outcome::success(Sr25519Signature{})));

  EXPECT_CALL(*block_tree, getBestContaining(best_block_info.hash, _))
      .WillOnce(Return(best_block_info))
      .WillOnce(Return(
          BlockInfo(new_block.header.number, new_block.header.hash(*hasher))));

  EXPECT_CALL(*block_tree, addBlock(_)).WillOnce(Return(outcome::success()));

  //  EXPECT_CALL(*block_announce_transmitter_, blockAnnounce(_))
  //      .WillOnce(CheckBlockHeader(created_block_.header));

  ASSERT_OUTCOME_SUCCESS_TRY(babe->processSlot(slot, best_block_info));
}

///**
// * @given BABE production
// * @when not in authority list
// * @then next epoch is scheduled
// */
// TEST_F(BabeTest, NoSlotLeader) {
//  EXPECT_CALL(*clock_, now());
//  EXPECT_CALL(*slots_util_, slotFinishTime(_)).Times(testing::AnyNumber());
//
//  EXPECT_CALL(*block_tree_, bestLeaf()).WillRepeatedly(Return(best_leaf));
//
//  babe_config_->authorities.clear();
//  EXPECT_CALL(*timer_, expiresAt(_));
//  EXPECT_CALL(*timer_, asyncWait(_));
//
//  babe_->runEpoch();
//}
//
///**
// * @given BABE production
// * @when not in authority list
// * @then next epoch is scheduled
// */
// TEST_F(BabeTest, SlotLeader) {
//  EXPECT_CALL(*clock_, now());
//  EXPECT_CALL(*slots_util_, slotFinishTime(_)).Times(testing::AnyNumber());
//
//  EXPECT_CALL(*block_tree_, bestLeaf()).WillRepeatedly(Return(best_leaf));
//
//  babe_config_->authorities.clear();
//  EXPECT_CALL(*timer_, expiresAt(_));
//  EXPECT_CALL(*timer_, asyncWait(_));
//
//  babe_->runEpoch();
//}

// #include <boost/asio/io_context.hpp>
//
// #include "consensus/babe/types/seal.hpp"
// #include "mock/core/application/app_configuration_mock.hpp"
// #include "mock/core/application/app_state_manager_mock.hpp"
// #include "mock/core/authorship/proposer_mock.hpp"
// #include "mock/core/blockchain/block_tree_mock.hpp"
// #include "mock/core/blockchain/digest_tracker_mock.hpp"
// #include "mock/core/clock/clock_mock.hpp"
// #include "mock/core/clock/timer_mock.hpp"
// #include "mock/core/consensus/babe/babe_config_repository_mock.hpp"
// #include "mock/core/consensus/babe_lottery_mock.hpp"
// #include "mock/core/consensus/grandpa/grandpa_mock.hpp"
// #include "mock/core/consensus/timeline/block_executor_mock.hpp"
// #include "mock/core/consensus/timeline/consistency_keeper_mock.hpp"
// #include "mock/core/consensus/timeline/slots_util_mock.hpp"
// #include "mock/core/consensus/validation/block_validator_mock.hpp"
// #include "mock/core/crypto/hasher_mock.hpp"
// #include "mock/core/crypto/session_keys_mock.hpp"
// #include "mock/core/crypto/sr25519_provider_mock.hpp"
// #include "mock/core/dispute_coordinator/dispute_coordinator_mock.hpp"
// #include "mock/core/network/block_announce_transmitter_mock.hpp"
// #include "mock/core/network/synchronizer_mock.hpp"
// #include "mock/core/parachain/backing_store_mock.hpp"
// #include "mock/core/parachain/bitfield_store_mock.hpp"
// #include "mock/core/runtime/core_mock.hpp"
// #include "mock/core/runtime/offchain_worker_api_mock.hpp"
// #include "mock/core/storage/trie/trie_storage_mock.hpp"
// #include "mock/core/transaction_pool/transaction_pool_mock.hpp"
// #include "runtime/runtime_context.hpp"
// #include "storage/trie/serialization/ordered_trie_hash.hpp"
// #include "testutil/lazy.hpp"
// #include "testutil/literals.hpp"
// #include "testutil/prepare_loggers.hpp"
// #include "testutil/sr25519_utils.hpp"
//
// using namespace kagome;
// using namespace consensus;
// using namespace babe;
// using namespace grandpa;
// using namespace application;
// using namespace blockchain;
// using namespace authorship;
// using namespace crypto;
// using namespace primitives;
// using namespace clock;
// using namespace common;
// using namespace network;
//
// using testing::_;
// using testing::A;
// using testing::Ref;
// using testing::Return;
// using std::chrono_literals::operator""ms;
//
//// TODO (kamilsa): workaround unless we bump gtest version to 1.8.1+
// namespace kagome::primitives {
//   std::ostream &operator<<(std::ostream &s,
//                            const detail::DigestItemCommon &dic) {
//     return s;
//   }
// }  // namespace kagome::primitives
//
// static Digest make_digest(SlotNumber slot) {
//   Digest digest;
//
//   BabeBlockHeader babe_header{
//       .slot_assignment_type = SlotType::SecondaryPlain,
//       .authority_index = 0,
//       .slot_number = slot,
//   };
//   common::Buffer encoded_header{scale::encode(babe_header).value()};
//   digest.emplace_back(
//       primitives::PreRuntime{{primitives::kBabeEngineId, encoded_header}});
//
//   consensus::babe::Seal seal{};
//   common::Buffer encoded_seal{scale::encode(seal).value()};
//   digest.emplace_back(
//       primitives::Seal{{primitives::kBabeEngineId, encoded_seal}});
//
//   return digest;
// };
//
// class BabeTest : public testing::Test {
//  public:
//   static void SetUpTestCase() {
//     testutil::prepareLoggers();
//   }
//
//   void SetUp() override {
//     app_state_manager_ = std::make_shared<AppStateManagerMock>();
//     lottery_ = std::make_shared<BabeLotteryMock>();
//     synchronizer_ = std::make_shared<network::SynchronizerMock>();
//     babe_block_validator_ = std::make_shared<BlockValidatorMock>();
//     grandpa_ = std::make_shared<GrandpaMock>();
//     tx_pool_ = std::make_shared<transaction_pool::TransactionPoolMock>();
//     core_ = std::make_shared<runtime::CoreMock>();
//     proposer_ = std::make_shared<ProposerMock>();
//     block_tree_ = std::make_shared<BlockTreeMock>();
//     block_announce_transmitter_ =
//         std::make_shared<BlockAnnounceTransmitterMock>();
//     clock_ = std::make_shared<SystemClockMock>();
//     hasher_ = std::make_shared<HasherMock>();
//     timer_mock_ = std::make_unique<testutil::TimerMock>();
//     timer_ = timer_mock_.get();
//     digest_tracker_ = std::make_shared<DigestTrackerMock>();
//     ON_CALL(*digest_tracker_, onDigest(_, _))
//         .WillByDefault(Return(outcome::success()));
//
//     io_context_ = std::make_shared<boost::asio::io_context>();
//
//     // add initialization logic
//     babe_config_ = std::make_shared<primitives::BabeConfiguration>();
//     babe_config_->slot_duration = 60ms;
//     babe_config_->randomness.fill(0);
//     babe_config_->authorities = {
//         primitives::Authority{{keypair_->public_key}, 1}};
//     babe_config_->leadership_rate = {1, 4};
//     babe_config_->epoch_length = 2;
//
//     babe_config_repo_ = std::make_shared<BabeConfigRepositoryMock>();
//     ON_CALL(*babe_config_repo_, config(_, _))
//         .WillByDefault(Return(babe_config_));
//     ON_CALL(*babe_config_repo_, epochLength())
//         .WillByDefault(Return(babe_config_->epoch_length));
//
//     slots_util_ = std::make_shared<SlotsUtilMock>();
//     EXPECT_CALL(*slots_util_, slotToEpochDescriptor(_, _))
//         .WillRepeatedly(Return(EpochDescriptor{}));
//
//     storage_sub_engine_ =
//         std::make_shared<primitives::events::StorageSubscriptionEngine>();
//     chain_events_engine_ =
//         std::make_shared<primitives::events::ChainSubscriptionEngine>();
//
//     offchain_worker_api_ =
//     std::make_shared<runtime::OffchainWorkerApiMock>();
//     EXPECT_CALL(*offchain_worker_api_, offchain_worker(_, _))
//         .WillRepeatedly(Return(outcome::success()));
//
//     core_ = std::make_shared<runtime::CoreMock>();
//     consistency_keeper_ = std::make_shared<ConsistencyKeeperMock>();
//
//     trie_storage_ = std::make_shared<storage::trie::TrieStorageMock>();
//
//     auto block_executor = std::make_shared<BlockExecutorMock>();
//
//     EXPECT_CALL(*app_state_manager_,
//     atPrepare(_)).Times(testing::AnyNumber());
//     EXPECT_CALL(*app_state_manager_,
//     atLaunch(_)).Times(testing::AnyNumber());
//     EXPECT_CALL(*app_state_manager_,
//     atShutdown(_)).Times(testing::AnyNumber());
//
//     auto sr25519_provider = std::make_shared<Sr25519ProviderMock>();
//     EXPECT_CALL(*sr25519_provider, sign(_, _))
//         .WillRepeatedly(Return(Sr25519Signature{}));
//
//     bitfield_store_ =
//     std::make_shared<kagome::parachain::BitfieldStoreMock>(); backing_store_
//     = std::make_shared<kagome::parachain::BackingStoreMock>();
//     babe_status_observable_ =
//         std::make_shared<primitives::events::BabeStateSubscriptionEngine>();
//
//     session_keys_ = std::make_shared<SessionKeysMock>();
//     EXPECT_CALL(*session_keys_, getBabeKeyPair(_))
//         .WillRepeatedly(Return(std::make_pair(keypair_, 0)));
//
//     dispute_coordinator_ =
//     std::make_shared<dispute::DisputeCoordinatorMock>();
//     ON_CALL(*dispute_coordinator_, getDisputeForInherentData(_, _))
//         .WillByDefault(testing::WithArg<1>(testing::Invoke(
//             [](const auto &f) { f(dispute::MultiDisputeStatementSet()); })));
//
//     babe_ = std::make_shared<babe::BabeImpl>(
//         app_config_,
//         app_state_manager_,
//         lottery_,
//         babe_config_repo_,
//         proposer_,
//         block_tree_,
//         block_announce_transmitter_,
//         sr25519_provider,
//         session_keys_,
//         clock_,
//         hasher_,
//         std::move(timer_mock_),
//         digest_tracker_,
//         // safe null, because object is not used during test
//         nullptr,
//         testutil::sptr_to_lazy<network::WarpProtocol>(warp_protocol_),
//         grandpa_,
//         synchronizer_,
//         slots_util_,
//         bitfield_store_,
//         backing_store_,
//         storage_sub_engine_,
//         chain_events_engine_,
//         offchain_worker_api_,
//         core_,
//         consistency_keeper_,
//         trie_storage_,
//         babe_status_observable_,
//         dispute_coordinator_);
//
//     epoch_ = 0;
//
//     // add extrinsics root to the header
//     std::vector<common::Buffer> encoded_exts(
//         {common::Buffer(scale::encode(extrinsic_).value())});
//     created_block_.header.extrinsics_root =
//         common::Hash256::fromSpan(
//             kagome::storage::trie::calculateOrderedTrieHash(
//                 storage::trie::StateVersion::V0,
//                 encoded_exts.begin(),
//                 encoded_exts.end())
//                 .value())
//             .value();
//   }
//
//   application::AppConfigurationMock app_config_;
//   std::shared_ptr<AppStateManagerMock> app_state_manager_;
//   std::shared_ptr<BabeLotteryMock> lottery_;
//   std::shared_ptr<Synchronizer> synchronizer_;
//   std::shared_ptr<BlockValidator> babe_block_validator_;
//   std::shared_ptr<GrandpaMock> grandpa_;
//   std::shared_ptr<runtime::CoreMock> core_;
//   std::shared_ptr<ProposerMock> proposer_;
//   std::shared_ptr<BlockTreeMock> block_tree_;
//   std::shared_ptr<transaction_pool::TransactionPoolMock> tx_pool_;
//   std::shared_ptr<BlockAnnounceTransmitterMock> block_announce_transmitter_;
//   std::shared_ptr<Sr25519Keypair> keypair_ =
//       std::make_shared<Sr25519Keypair>(generateSr25519Keypair());
//   std::shared_ptr<SessionKeysMock> session_keys_;
//   std::shared_ptr<SystemClockMock> clock_;
//   std::shared_ptr<HasherMock> hasher_;
//   std::unique_ptr<testutil::TimerMock> timer_mock_;
//   testutil::TimerMock *timer_;
//   std::shared_ptr<DigestTrackerMock> digest_tracker_;
//   std::shared_ptr<network::WarpProtocol> warp_protocol_;
//   std::shared_ptr<primitives::BabeConfiguration> babe_config_;
//   std::shared_ptr<BabeConfigRepositoryMock> babe_config_repo_;
//   std::shared_ptr<SlotsUtilMock> slots_util_;
//   primitives::events::StorageSubscriptionEnginePtr storage_sub_engine_;
//   primitives::events::ChainSubscriptionEnginePtr chain_events_engine_;
//   std::shared_ptr<runtime::OffchainWorkerApiMock> offchain_worker_api_;
//   std::shared_ptr<ConsistencyKeeperMock> consistency_keeper_;
//   std::shared_ptr<storage::trie::TrieStorageMock> trie_storage_;
//   std::shared_ptr<boost::asio::io_context> io_context_;
//   std::shared_ptr<kagome::parachain::BitfieldStoreMock> bitfield_store_;
//   std::shared_ptr<kagome::parachain::BackingStoreMock> backing_store_;
//   primitives::events::BabeStateSubscriptionEnginePtr babe_status_observable_;
//   std::shared_ptr<dispute::DisputeCoordinatorMock> dispute_coordinator_;
//
//   std::shared_ptr<babe::BabeImpl> babe_;
//
//   EpochNumber epoch_;
//
//   VRFOutput leader_vrf_output_{uint256_to_le_bytes(50), {}};
//   std::array<std::optional<VRFOutput>, 2> leadership_{std::nullopt,
//                                                       leader_vrf_output_};
//
//   BlockHash best_block_hash_ = "block#0"_hash256;
//   BlockNumber best_block_number_ = 0u;
//   BlockHeader best_block_header_{
//       best_block_number_,          // number
//       {},                          // parent
//       "state_root#0"_hash256,      // state_root
//       "extrinsic_root#0"_hash256,  // extrinsic_root
//       {}                           // digest
//   };
//
//   primitives::BlockInfo best_leaf{best_block_number_, best_block_hash_};
//
//   BlockHeader block_header_{
//       best_block_number_ + 1,      // number
//       best_block_hash_,            // parent
//       "state_root#1"_hash256,      // state_root
//       "extrinsic_root#1"_hash256,  // extrinsic_root
//       make_digest(0)               // digest
//   };
//
//   Extrinsic extrinsic_{{1, 2, 3}};
//   Block created_block_{block_header_, {extrinsic_}};
//
//   Hash256 created_block_hash_{"block#1"_hash256};
// };
//
// ACTION_P(CheckBlockHeader, expected_block_header) {
//   auto header_to_check = arg0.header;
//   ASSERT_EQ(header_to_check.digest.size(), 3);
//   header_to_check.digest.pop_back();
//   ASSERT_EQ(header_to_check, expected_block_header);
// }
//
///**
// * @given BABE production
// * @when running it in epoch with two slots @and out node is a leader in one
// of
// * them
// * @then block is emitted in the leader slot @and after two slots BABE moves
// to
// * the next epoch
// */
// TEST_F(BabeTest, Success) {
//  Randomness randomness;
//  auto undefined_epoch = std::numeric_limits<EpochNumber>::max();
//  EXPECT_CALL(*lottery_, getEpoch())
//      .WillOnce(Return(undefined_epoch))
//      .WillOnce(Return(epoch_));
//  EXPECT_CALL(*lottery_, changeEpoch(epoch_, randomness, _, *keypair_))
//      .Times(1);
//  EXPECT_CALL(*lottery_, getSlotLeadership(_))
//      .WillOnce(Return(leadership_[0]))
//      .WillOnce(Return(leadership_[1]));
//
//  EXPECT_CALL(*clock_, now())
//      .WillRepeatedly(Return(clock::SystemClockMock::zero()));
//
//  EXPECT_CALL(*babe_config_repo_, slotDuration()).WillRepeatedly(Return(1ms));
//  EXPECT_CALL(*slots_util_, slotFinishTime(_))
//      .WillRepeatedly(Return(clock::SystemClockMock::zero()
//                             + babe_config_repo_->slotDuration()));
//
//  testing::Sequence s;
//  auto breaker = [](const boost::system::error_code &ec) {
//    throw std::logic_error("Must not be called");
//  };
//  std::function<void(const boost::system::error_code &ec)> on_run_slot_2 =
//      breaker;
//  std::function<void(const boost::system::error_code &ec)> on_run_slot_3 =
//      breaker;
//  EXPECT_CALL(*timer_, asyncWait(_))
//      .InSequence(s)
//      .WillOnce(testing::SaveArg<0>(&on_run_slot_2))
//      .WillOnce(testing::SaveArg<0>(&on_run_slot_3));
//  EXPECT_CALL(*timer_, expiresAt(_)).WillRepeatedly(Return());
//
//  // processSlotLeadership
//  // we are not leader of the first slot, but leader of the second
//  EXPECT_CALL(*block_tree_, bestLeaf()).WillRepeatedly(Return(best_leaf));
//
//  // call for check condition of offchain worker run
//  EXPECT_CALL(*block_tree_, getLastFinalized())
//      .WillRepeatedly(Return(best_leaf));
//  EXPECT_CALL(*block_tree_, getBestContaining(_, _))
//      .WillOnce(Return(best_leaf))
//      .WillOnce(
//          Return(BlockInfo(created_block_.header.number,
//          created_block_hash_)));
//
//  EXPECT_CALL(*block_tree_, getBlockHeader(best_block_hash_))
//      .WillRepeatedly(Return(outcome::success(best_block_header_)));
//  EXPECT_CALL(*block_tree_, getBlockHeader(created_block_hash_))
//      .WillRepeatedly(Return(outcome::success(block_header_)));
//
//  EXPECT_CALL(*proposer_, propose(best_leaf, _, _, _, _))
//      .WillOnce(Return(created_block_));
//
//  EXPECT_CALL(*hasher_, blake2b_256(_))
//      .WillRepeatedly(Return(created_block_hash_));
//  EXPECT_CALL(*block_tree_, addBlock(_)).WillOnce(Return(outcome::success()));
//
//  EXPECT_CALL(*block_announce_transmitter_, blockAnnounce(_))
//      .WillOnce(CheckBlockHeader(created_block_.header));
//
//  babe_->runEpoch();
//  ASSERT_NO_THROW(on_run_slot_2({}));
//}
//
///**
// * @given BABE production
// * @when not in authority list
// * @then next epoch is scheduled
// */
// TEST_F(BabeTest, NotAuthority) {
//  EXPECT_CALL(*clock_, now());
//  EXPECT_CALL(*slots_util_, slotFinishTime(_)).Times(testing::AnyNumber());
//
//  EXPECT_CALL(*block_tree_, bestLeaf()).WillRepeatedly(Return(best_leaf));
//
//  babe_config_->authorities.clear();
//  EXPECT_CALL(*timer_, expiresAt(_));
//  EXPECT_CALL(*timer_, asyncWait(_));
//
//  babe_->runEpoch();
//}
