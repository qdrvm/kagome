/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>

#include "consensus/babe/impl/babe_impl.hpp"
#include "consensus/babe/types/seal.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/authorship/proposer_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/blockchain/digest_tracker_mock.hpp"
#include "mock/core/clock/clock_mock.hpp"
#include "mock/core/clock/timer_mock.hpp"
#include "mock/core/consensus/babe/babe_config_repository_mock.hpp"
#include "mock/core/consensus/babe/babe_util_mock.hpp"
#include "mock/core/consensus/babe/block_executor_mock.hpp"
#include "mock/core/consensus/babe/consistency_keeper_mock.hpp"
#include "mock/core/consensus/babe_lottery_mock.hpp"
#include "mock/core/consensus/grandpa/environment_mock.hpp"
#include "mock/core/consensus/validation/block_validator_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/network/block_announce_transmitter_mock.hpp"
#include "mock/core/network/synchronizer_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/runtime/offchain_worker_api_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "mock/core/transaction_pool/transaction_pool_mock.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/sr25519_utils.hpp"

using namespace kagome;
using namespace consensus;
using namespace babe;
using namespace grandpa;
using namespace application;
using namespace blockchain;
using namespace authorship;
using namespace crypto;
using namespace primitives;
using namespace clock;
using namespace common;
using namespace network;

using testing::_;
using testing::A;
using testing::Ref;
using testing::Return;
using testing::ReturnRef;
using std::chrono_literals::operator""ms;

// TODO (kamilsa): workaround unless we bump gtest version to 1.8.1+
namespace kagome::primitives {
  std::ostream &operator<<(std::ostream &s,
                           const detail::DigestItemCommon &dic) {
    return s;
  }
}  // namespace kagome::primitives

static Digest make_digest(BabeSlotNumber slot) {
  Digest digest;

  BabeBlockHeader babe_header{
      .slot_assignment_type = SlotType::SecondaryPlain,
      .authority_index = 0,
      .slot_number = slot,
  };
  common::Buffer encoded_header{scale::encode(babe_header).value()};
  digest.emplace_back(
      primitives::PreRuntime{{primitives::kBabeEngineId, encoded_header}});

  consensus::babe::Seal seal{};
  common::Buffer encoded_seal{scale::encode(seal).value()};
  digest.emplace_back(
      primitives::Seal{{primitives::kBabeEngineId, encoded_seal}});

  return digest;
};

class BabeTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    app_state_manager_ = std::make_shared<AppStateManagerMock>();
    lottery_ = std::make_shared<BabeLotteryMock>();
    synchronizer_ = std::make_shared<network::SynchronizerMock>();
    babe_block_validator_ = std::make_shared<BlockValidatorMock>();
    grandpa_environment_ = std::make_shared<grandpa::EnvironmentMock>();
    tx_pool_ = std::make_shared<transaction_pool::TransactionPoolMock>();
    core_ = std::make_shared<runtime::CoreMock>();
    proposer_ = std::make_shared<ProposerMock>();
    block_tree_ = std::make_shared<BlockTreeMock>();
    block_announce_transmitter_ =
        std::make_shared<BlockAnnounceTransmitterMock>();
    clock_ = std::make_shared<SystemClockMock>();
    hasher_ = std::make_shared<HasherMock>();
    timer_mock_ = std::make_unique<testutil::TimerMock>();
    timer_ = timer_mock_.get();
    digest_tracker_ = std::make_shared<DigestTrackerMock>();
    ON_CALL(*digest_tracker_, onDigest(_, _))
        .WillByDefault(Return(outcome::success()));

    io_context_ = std::make_shared<boost::asio::io_context>();

    // add initialization logic
    babe_config_ = std::make_shared<primitives::BabeConfiguration>();
    babe_config_->slot_duration = 60ms;
    babe_config_->randomness.fill(0);
    babe_config_->authorities = {
        primitives::Authority{{keypair_->public_key}, 1}};
    babe_config_->leadership_rate = {1, 4};
    babe_config_->epoch_length = 2;

    babe_config_repo_ = std::make_shared<BabeConfigRepositoryMock>();
    ON_CALL(*babe_config_repo_, config(_, _))
        .WillByDefault(Return(babe_config_));
    ON_CALL(*babe_config_repo_, epochLength())
        .WillByDefault(Return(babe_config_->epoch_length));

    babe_util_ = std::make_shared<BabeUtilMock>();
    EXPECT_CALL(*babe_util_, slotToEpoch(_)).WillRepeatedly(Return(0));

    chain_events_engine_ =
        std::make_shared<primitives::events::ChainSubscriptionEngine>();

    offchain_worker_api_ = std::make_shared<runtime::OffchainWorkerApiMock>();
    EXPECT_CALL(*offchain_worker_api_, offchain_worker(_, _))
        .WillRepeatedly(Return(outcome::success()));

    core_ = std::make_shared<runtime::CoreMock>();
    consistency_keeper_ = std::make_shared<babe::ConsistencyKeeperMock>();

    trie_storage_ = std::make_shared<storage::trie::TrieStorageMock>();

    auto block_executor = std::make_shared<BlockExecutorMock>();

    EXPECT_CALL(*app_state_manager_, atPrepare(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*app_state_manager_, atLaunch(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*app_state_manager_, atShutdown(_)).Times(testing::AnyNumber());

    auto sr25519_provider = std::make_shared<Sr25519ProviderMock>();
    EXPECT_CALL(*sr25519_provider, sign(_, _))
        .WillRepeatedly(Return(Sr25519Signature{}));

    babe_ = std::make_shared<babe::BabeImpl>(app_config_,
                                             app_state_manager_,
                                             lottery_,
                                             babe_config_repo_,
                                             proposer_,
                                             block_tree_,
                                             block_announce_transmitter_,
                                             sr25519_provider,
                                             keypair_,
                                             clock_,
                                             hasher_,
                                             std::move(timer_mock_),
                                             digest_tracker_,
                                             synchronizer_,
                                             babe_util_,
                                             chain_events_engine_,
                                             offchain_worker_api_,
                                             core_,
                                             consistency_keeper_,
                                             trie_storage_);

    epoch_.start_slot = 0;
    epoch_.epoch_number = 0;

    // add extrinsics root to the header
    std::vector<common::Buffer> encoded_exts(
        {common::Buffer(scale::encode(extrinsic_).value())});
    created_block_.header.extrinsics_root =
        common::Hash256::fromSpan(
            kagome::storage::trie::calculateOrderedTrieHash(
                storage::trie::StateVersion::TODO_NotSpecified,
                encoded_exts.begin(),
                encoded_exts.end())
                .value())
            .value();
  }

  application::AppConfigurationMock app_config_;
  std::shared_ptr<AppStateManagerMock> app_state_manager_;
  std::shared_ptr<BabeLotteryMock> lottery_;
  std::shared_ptr<Synchronizer> synchronizer_;
  std::shared_ptr<BlockValidator> babe_block_validator_;
  std::shared_ptr<grandpa::EnvironmentMock> grandpa_environment_;
  std::shared_ptr<runtime::CoreMock> core_;
  std::shared_ptr<ProposerMock> proposer_;
  std::shared_ptr<BlockTreeMock> block_tree_;
  std::shared_ptr<transaction_pool::TransactionPoolMock> tx_pool_;
  std::shared_ptr<BlockAnnounceTransmitterMock> block_announce_transmitter_;
  std::shared_ptr<Sr25519Keypair> keypair_ =
      std::make_shared<Sr25519Keypair>(generateSr25519Keypair());
  std::shared_ptr<SystemClockMock> clock_;
  std::shared_ptr<HasherMock> hasher_;
  std::unique_ptr<testutil::TimerMock> timer_mock_;
  testutil::TimerMock *timer_;
  std::shared_ptr<DigestTrackerMock> digest_tracker_;
  std::shared_ptr<primitives::BabeConfiguration> babe_config_;
  std::shared_ptr<BabeConfigRepositoryMock> babe_config_repo_;
  std::shared_ptr<BabeUtilMock> babe_util_;
  primitives::events::ChainSubscriptionEnginePtr chain_events_engine_;
  std::shared_ptr<runtime::OffchainWorkerApiMock> offchain_worker_api_;
  std::shared_ptr<babe::ConsistencyKeeperMock> consistency_keeper_;
  std::shared_ptr<storage::trie::TrieStorageMock> trie_storage_;
  std::shared_ptr<boost::asio::io_context> io_context_;

  std::shared_ptr<babe::BabeImpl> babe_;

  EpochDescriptor epoch_;

  VRFOutput leader_vrf_output_{uint256_to_le_bytes(50), {}};
  std::array<std::optional<VRFOutput>, 2> leadership_{std::nullopt,
                                                      leader_vrf_output_};

  BlockHash best_block_hash_ = "block#0"_hash256;
  BlockNumber best_block_number_ = 0u;
  BlockHeader best_block_header_{.parent_hash = {},
                                 .number = best_block_number_,
                                 .state_root = "state_root#0"_hash256,
                                 .extrinsics_root = "extrinsic_root#0"_hash256,
                                 .digest = {}};

  primitives::BlockInfo best_leaf{best_block_number_, best_block_hash_};

  BlockHeader block_header_{.parent_hash = best_block_hash_,
                            .number = best_block_number_ + 1,
                            .state_root = "state_root#1"_hash256,
                            .extrinsics_root = "extrinsic_root#1"_hash256,
                            .digest = make_digest(0)};
  Extrinsic extrinsic_{{1, 2, 3}};
  Block created_block_{block_header_, {extrinsic_}};

  Hash256 created_block_hash_{"block#1"_hash256};

  consensus::babe::EpochDigest expected_epoch_digest;
};

ACTION_P(CheckBlockHeader, expected_block_header) {
  auto header_to_check = arg0.header;
  ASSERT_EQ(header_to_check.digest.size(), 3);
  header_to_check.digest.pop_back();
  ASSERT_EQ(header_to_check, expected_block_header);
}

/**
 * @given BABE production
 * @when running it in epoch with two slots @and out node is a leader in one of
 * them
 * @then block is emitted in the leader slot @and after two slots BABE moves to
 * the next epoch
 */
TEST_F(BabeTest, Success) {
  Randomness randomness;
  EXPECT_CALL(*lottery_, getEpoch())
      .WillOnce(
          Return(EpochDescriptor{0, std::numeric_limits<uint64_t>::max()}))
      .WillOnce(Return(epoch_));
  EXPECT_CALL(*lottery_, changeEpoch(epoch_, randomness, _, *keypair_))
      .Times(1);
  EXPECT_CALL(*lottery_, getSlotLeadership(_))
      .WillOnce(Return(leadership_[0]))
      .WillOnce(Return(leadership_[1]));

  EXPECT_CALL(*clock_, now())
      .WillRepeatedly(Return(clock::SystemClockMock::zero()));

  EXPECT_CALL(*babe_config_repo_, slotDuration()).WillRepeatedly(Return(1ms));
  EXPECT_CALL(*babe_util_, slotStartTime(_))
      .WillRepeatedly(Return(clock::SystemClockMock::zero()));
  EXPECT_CALL(*babe_util_, slotFinishTime(_))
      .WillRepeatedly(Return(clock::SystemClockMock::zero()));
  EXPECT_CALL(*babe_util_, remainToStartOfSlot(_)).WillRepeatedly(Return(1ms));
  EXPECT_CALL(*babe_util_, remainToFinishOfSlot(_)).WillRepeatedly(Return(1ms));
  EXPECT_CALL(*babe_util_, syncEpoch(_)).Times(testing::AnyNumber());

  testing::Sequence s;
  auto breaker = [](const std::error_code &ec) {
    throw std::logic_error("Must not be called");
  };
  std::function<void(const std::error_code &ec)> on_process_slot_1 = breaker;
  std::function<void(const std::error_code &ec)> on_run_slot_2 = breaker;
  std::function<void(const std::error_code &ec)> on_process_slot_2 = breaker;
  std::function<void(const std::error_code &ec)> on_run_slot_3 = breaker;
  EXPECT_CALL(*timer_, asyncWait(_))
      .InSequence(s)
      .WillOnce(testing::SaveArg<0>(&on_process_slot_1))
      .WillOnce(testing::SaveArg<0>(&on_run_slot_2))
      .WillOnce(testing::SaveArg<0>(&on_process_slot_2))
      .WillOnce(testing::SaveArg<0>(&on_run_slot_3));
  EXPECT_CALL(*timer_, expiresAt(_)).WillRepeatedly(Return());

  // processSlotLeadership
  // we are not leader of the first slot, but leader of the second
  EXPECT_CALL(*block_tree_, deepestLeaf()).WillRepeatedly(Return(best_leaf));

  // call for check condition of offchain worker run
  EXPECT_CALL(*block_tree_, getLastFinalized())
      .WillRepeatedly(Return(best_leaf));
  EXPECT_CALL(*block_tree_, getBestContaining(_, _))
      .WillOnce(Return(best_leaf))
      .WillOnce(
          Return(BlockInfo(created_block_.header.number, created_block_hash_)));

  EXPECT_CALL(*block_tree_, getBlockHeader(BlockId(best_block_hash_)))
      .WillRepeatedly(Return(outcome::success(best_block_header_)));
  EXPECT_CALL(*block_tree_, getBlockHeader(BlockId(BlockNumber(1))))
      .WillRepeatedly(Return(outcome::success(block_header_)));

  EXPECT_CALL(*proposer_, propose(best_leaf, _, _))
      .WillOnce(Return(created_block_));

  EXPECT_CALL(*hasher_, blake2b_256(_))
      .WillRepeatedly(Return(created_block_hash_));
  EXPECT_CALL(*block_tree_, addBlock(_)).WillOnce(Return(outcome::success()));

  EXPECT_CALL(*block_announce_transmitter_, blockAnnounce(_))
      .WillOnce(CheckBlockHeader(created_block_.header));

  babe_->runEpoch(epoch_);
  ASSERT_NO_THROW(on_process_slot_1({}));
  ASSERT_NO_THROW(on_run_slot_2({}));
  ASSERT_NO_THROW(on_process_slot_2({}));
}

/**
 * @given BABE production
 * @when not in authority list
 * @then next epoch is scheduled
 */
TEST_F(BabeTest, NotAuthority) {
  EXPECT_CALL(*clock_, now());
  EXPECT_CALL(*babe_config_repo_, slotDuration());
  EXPECT_CALL(*babe_util_, slotFinishTime(_)).Times(testing::AnyNumber());
  EXPECT_CALL(*babe_util_, syncEpoch(_));
  EXPECT_CALL(*timer_, expiresAt(_));
  std::function<void(const std::error_code &)> process_slot;
  EXPECT_CALL(*timer_, asyncWait(_))
      .WillOnce(testing::SaveArg<0>(&process_slot));
  babe_->runEpoch(epoch_);

  EXPECT_CALL(*block_tree_, deepestLeaf()).WillRepeatedly(Return(best_leaf));
  EXPECT_CALL(*block_tree_, getBlockHeader(BlockId(best_block_hash_)))
      .WillOnce(Return(best_block_header_));
  EXPECT_CALL(*babe_util_, syncEpoch(_));
  EXPECT_CALL(*babe_util_, slotStartTime(_));

  expected_epoch_digest.authorities.clear();
  EXPECT_CALL(*timer_, expiresAt(_));
  EXPECT_CALL(*timer_, asyncWait(_));
  process_slot({});
}
