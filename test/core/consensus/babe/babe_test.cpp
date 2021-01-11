/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>
#include <chrono>
#include <memory>

#include "clock/impl/clock_impl.hpp"
#include "consensus/babe/babe_error.hpp"
#include "consensus/babe/impl/babe_impl.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/authorship/proposer_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/clock/clock_mock.hpp"
#include "mock/core/clock/timer_mock.hpp"
#include "mock/core/consensus/authority/authority_update_observer_mock.hpp"
#include "mock/core/consensus/babe/babe_gossiper_mock.hpp"
#include "mock/core/consensus/babe/babe_synchronizer_mock.hpp"
#include "mock/core/consensus/babe/epoch_storage_mock.hpp"
#include "mock/core/consensus/babe_lottery_mock.hpp"
#include "mock/core/consensus/validation/block_validator_mock.hpp"
#include "mock/core/consensus/validation/justification_validator.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/runtime/babe_api_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "mock/core/transaction_pool/transaction_pool_mock.hpp"
#include "primitives/block.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"
#include "testutil/literals.hpp"
#include "testutil/sr25519_utils.hpp"

using namespace kagome;
using namespace consensus;
using namespace authority;
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

Hash256 createHash(uint8_t byte) {
  Hash256 h;
  h.fill(byte);
  return h;
}

// TODO (kamilsa): workaround unless we bump gtest version to 1.8.1+
namespace kagome::primitives {
  std::ostream &operator<<(std::ostream &s,
                           const detail::DigestItemCommon &dic) {
    return s;
  }
}  // namespace kagome::primitives

class BabeTest : public testing::Test {
 public:
  void SetUp() override {
    app_state_manager_ = std::make_shared<AppStateManagerMock>();
    lottery_ = std::make_shared<BabeLotteryMock>();
    babe_synchronizer_ = std::make_shared<BabeSynchronizerMock>();
    trie_db_ = std::make_shared<storage::trie::TrieStorageMock>();
    babe_block_validator_ = std::make_shared<BlockValidatorMock>();
    justification_validator_ = std::make_shared<JustificationValidatorMock>();
    epoch_storage_ = std::make_shared<EpochStorageMock>();
    tx_pool_ = std::make_shared<transaction_pool::TransactionPoolMock>();
    core_ = std::make_shared<runtime::CoreMock>();
    proposer_ = std::make_shared<ProposerMock>();
    block_tree_ = std::make_shared<BlockTreeMock>();
    gossiper_ = std::make_shared<BabeGossiperMock>();
    clock_ = std::make_shared<SystemClockMock>();
    hasher_ = std::make_shared<HasherMock>();
    timer_mock_ = std::make_unique<testutil::TimerMock>();
    timer_ = timer_mock_.get();
    grandpa_authority_update_observer_ =
        std::make_shared<AuthorityUpdateObserverMock>();

    // add initialization logic
    auto expected_config = std::make_shared<primitives::BabeConfiguration>();
    expected_config->slot_duration = slot_duration_;
    expected_config->randomness.fill(0);
    expected_config->genesis_authorities = {
        primitives::Authority{{keypair_.public_key}, 1}};
    expected_config->leadership_rate = {1, 4};
    expected_config->epoch_length = epoch_length_;

    consensus::NextEpochDescriptor expected_epoch_digest{
        .authorities = expected_config->genesis_authorities,
        .randomness = expected_config->randomness};
    EXPECT_CALL(*epoch_storage_, getEpochDescriptor(0))
        .WillRepeatedly(Return(expected_epoch_digest));
    EXPECT_CALL(*epoch_storage_, getEpochDescriptor(1))
        .WillRepeatedly(Return(expected_epoch_digest));

    auto block_executor =
        std::make_shared<BlockExecutor>(block_tree_,
                                        core_,
                                        expected_config,
                                        babe_synchronizer_,
                                        babe_block_validator_,
                                        justification_validator_,
                                        epoch_storage_,
                                        tx_pool_,
                                        hasher_,
                                        grandpa_authority_update_observer_,
                                        slots_strategy_);

    EXPECT_CALL(*app_state_manager_, atPrepare(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*app_state_manager_, atLaunch(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*app_state_manager_, atShutdown(_)).Times(testing::AnyNumber());

    auto sr25519_provider = std::make_shared<Sr25519ProviderMock>();
    EXPECT_CALL(*sr25519_provider, sign(_, _))
        .WillRepeatedly(Return(Sr25519Signature{}));

    babe_ = std::make_shared<BabeImpl>(app_state_manager_,
                                       lottery_,
                                       block_executor,
                                       trie_db_,
                                       epoch_storage_,
                                       expected_config,
                                       proposer_,
                                       block_tree_,
                                       gossiper_,
                                       sr25519_provider,
                                       keypair_,
                                       clock_,
                                       hasher_,
                                       std::move(timer_mock_),
                                       grandpa_authority_update_observer_,
                                       slots_strategy_);

    epoch_.randomness = expected_config->randomness;
    epoch_.epoch_length = expected_config->epoch_length;
    epoch_.authorities = expected_config->genesis_authorities;
    epoch_.start_slot = 0;
    epoch_.epoch_index = 0;

    // add extrinsics root to the header
    std::vector<common::Buffer> encoded_exts(
        {common::Buffer(scale::encode(extrinsic_).value())});
    created_block_.header.extrinsics_root =
        common::Hash256::fromSpan(
            kagome::storage::trie::calculateOrderedTrieHash(
                encoded_exts.begin(), encoded_exts.end())
                .value())
            .value();
  }

  std::shared_ptr<AppStateManagerMock> app_state_manager_;
  std::shared_ptr<BabeLotteryMock> lottery_;
  std::shared_ptr<BabeSynchronizer> babe_synchronizer_;
  std::shared_ptr<storage::trie::TrieStorageMock> trie_db_;
  std::shared_ptr<BlockValidator> babe_block_validator_;
  std::shared_ptr<EpochStorageMock> epoch_storage_;
  std::shared_ptr<runtime::CoreMock> core_;
  std::shared_ptr<ProposerMock> proposer_;
  std::shared_ptr<BlockTreeMock> block_tree_;
  std::shared_ptr<transaction_pool::TransactionPoolMock> tx_pool_;
  std::shared_ptr<BabeGossiperMock> gossiper_;
  Sr25519Keypair keypair_{generateSr25519Keypair()};
  std::shared_ptr<SystemClockMock> clock_;
  std::shared_ptr<HasherMock> hasher_;
  std::unique_ptr<testutil::TimerMock> timer_mock_;
  testutil::TimerMock *timer_;
  std::shared_ptr<AuthorityUpdateObserverMock>
      grandpa_authority_update_observer_;
  std::shared_ptr<JustificationValidator> justification_validator_;

  SlotsStrategy slots_strategy_{SlotsStrategy::FromZero};

  std::shared_ptr<BabeImpl> babe_;

  Epoch epoch_;
  BabeDuration slot_duration_{60ms};
  EpochLength epoch_length_{2};

  VRFOutput leader_vrf_output_{
      uint256_t_to_bytes(50),
      {0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33,
       0x44, 0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x44, 0x11, 0x22,
       0x33, 0x44, 0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x44}};
  BabeLottery::SlotsLeadership leadership_{boost::none, leader_vrf_output_};

  BlockHash best_block_hash_{{0x41, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x44,
                              0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x54,
                              0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x44,
                              0x11, 0x22, 0x33, 0x44, 0x11, 0x24, 0x33, 0x44}};
  BlockNumber best_block_number_ = 1u;

  primitives::BlockInfo best_leaf{best_block_number_, best_block_hash_};

  BlockHeader block_header_{
      createHash(0), 2, createHash(1), createHash(2), {PreRuntime{}}};
  Extrinsic extrinsic_{{1, 2, 3}};
  Block created_block_{block_header_, {extrinsic_}};

  Hash256 created_block_hash_{createHash(3)};

  SystemClockImpl real_clock_{};
};

ACTION_P(CheckBlockHeader, expected_block_header) {
  auto header_to_check = arg0.header;
  ASSERT_EQ(header_to_check.digest.size(), 2);
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
  auto test_begin = real_clock_.now();

  // runEpoch
  epoch_.randomness.fill(0);
  EXPECT_CALL(*lottery_, slotsLeadership(epoch_, _, keypair_))
      .WillOnce(Return(leadership_));
  Epoch next_epoch = epoch_;
  next_epoch.epoch_index++;
  next_epoch.start_slot += epoch_length_;

  EXPECT_CALL(*lottery_, slotsLeadership(next_epoch, _, keypair_))
      .WillOnce(Return(leadership_));

  EXPECT_CALL(*trie_db_, getRootHash())
      .WillRepeatedly(Return(common::Buffer{}));

  // runSlot (3 times)
  EXPECT_CALL(*clock_, now())
      .WillOnce(Return(test_begin))
      .WillOnce(Return(test_begin + slot_duration_))
      .WillOnce(Return(test_begin + slot_duration_))
      .WillOnce(Return(test_begin + slot_duration_ * 2))
      .WillOnce(Return(test_begin + slot_duration_ * 2));

  EXPECT_CALL(*timer_, expiresAt(test_begin + slot_duration_));
  EXPECT_CALL(*timer_, expiresAt(test_begin + slot_duration_ * 2));
  EXPECT_CALL(*timer_, expiresAt(test_begin + slot_duration_ * 3));

  EXPECT_CALL(*timer_, asyncWait(_))
      .WillOnce(testing::InvokeArgument<0>(boost::system::error_code{}))
      .WillOnce(testing::InvokeArgument<0>(boost::system::error_code{}))
      .WillOnce({});

  // processSlotLeadership
  // we are not leader of the first slot, but leader of the second
  EXPECT_CALL(*block_tree_, deepestLeaf()).WillOnce(Return(best_leaf));
  EXPECT_CALL(*proposer_, propose(BlockId{best_block_hash_}, _, _))
      .WillOnce(Return(created_block_));
  EXPECT_CALL(*hasher_, blake2b_256(_)).WillOnce(Return(created_block_hash_));
  EXPECT_CALL(*block_tree_, addBlock(_)).WillOnce(Return(outcome::success()));

  EXPECT_CALL(*gossiper_, blockAnnounce(_))
      .WillOnce(CheckBlockHeader(created_block_.header));

  EXPECT_CALL(*epoch_storage_, setLastEpoch(_))
      .WillOnce(Return(outcome::success()))
      .WillOnce(Return(outcome::success()));

  babe_->runEpoch(epoch_, test_begin + slot_duration_);
}
