/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>
#include <chrono>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <memory>

#include "clock/impl/clock_impl.hpp"
#include "consensus/babe/babe_error.hpp"
#include "consensus/babe/impl/babe_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "mock/core/authorship/proposer_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/clock/clock_mock.hpp"
#include "mock/core/clock/timer_mock.hpp"
#include "mock/core/consensus/babe/babe_synchronizer_mock.hpp"
#include "mock/core/consensus/babe/epoch_storage_mock.hpp"
#include "mock/core/consensus/babe_lottery_mock.hpp"
#include "mock/core/consensus/validation/block_validator_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/network/babe_gossiper_mock.hpp"
#include "mock/core/runtime/babe_api_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "primitives/block.hpp"
#include "testutil/sr25519_utils.hpp"

using namespace kagome;
using namespace consensus;
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
    lottery_ = std::make_shared<BabeLotteryMock>();
    babe_synchronizer_ = std::make_shared<BabeSynchronizerMock>();
    babe_block_validator_ = std::make_shared<BlockValidatorMock>();
    epoch_storage_ = std::make_shared<EpochStorageMock>();
    babe_api_ = std::make_shared<runtime::BabeApiMock>();
    core_ = std::make_shared<runtime::CoreMock>();
    proposer_ = std::make_shared<ProposerMock>();
    block_tree_ = std::make_shared<BlockTreeMock>();
    gossiper_ = std::make_shared<BabeGossiperMock>();
    clock_ = std::make_shared<SystemClockMock>();
    hasher_ = std::make_shared<HasherMock>();
    timer_mock_ = std::make_unique<testutil::TimerMock>();
    timer_ = timer_mock_.get();

    // add initialization logic
    primitives::BabeConfiguration expected_config;
    expected_config.slot_duration = slot_duration_;
    expected_config.randomness.fill(0);
    expected_config.genesis_authorities = {
        primitives::Authority{{keypair_.public_key}, 1}};
    expected_config.c = {1, 4};
    expected_config.epoch_length = 2;
    EXPECT_CALL(*babe_api_, configuration()).WillOnce(Return(expected_config));

    consensus::NextEpochDescriptor expected_epoch_digest{
        .authorities = expected_config.genesis_authorities,
        .randomness = expected_config.randomness};
    EXPECT_CALL(*epoch_storage_, addEpochDescriptor(0, expected_epoch_digest))
        .WillOnce(Return());
    EXPECT_CALL(*epoch_storage_, addEpochDescriptor(1, expected_epoch_digest))
        .WillOnce(Return());
    EXPECT_CALL(*epoch_storage_, getEpochDescriptor(1))
        .WillOnce(Return(expected_epoch_digest));

    babe_ = std::make_shared<BabeImpl>(lottery_,
                                       babe_synchronizer_,
                                       babe_block_validator_,
                                       epoch_storage_,
                                       babe_api_,
                                       core_,
                                       proposer_,
                                       block_tree_,
                                       gossiper_,
                                       keypair_,
                                       clock_,
                                       hasher_,
                                       std::move(timer_mock_));

    epoch_.randomness = expected_config.randomness;
    epoch_.epoch_duration = expected_config.epoch_length;
    epoch_.authorities = expected_config.genesis_authorities;
    epoch_.start_slot = 0;
    epoch_.epoch_index = 0;
  }

  std::shared_ptr<BabeLotteryMock> lottery_;
  std::shared_ptr<BabeSynchronizer> babe_synchronizer_;
  std::shared_ptr<BlockValidator> babe_block_validator_;
  std::shared_ptr<EpochStorageMock> epoch_storage_;
  std::shared_ptr<runtime::BabeApiMock> babe_api_;
  std::shared_ptr<runtime::CoreMock> core_;
  std::shared_ptr<ProposerMock> proposer_;
  std::shared_ptr<BlockTreeMock> block_tree_;
  std::shared_ptr<BabeGossiperMock> gossiper_;
  SR25519Keypair keypair_{generateSR25519Keypair()};
  std::shared_ptr<SystemClockMock> clock_;
  std::shared_ptr<HasherMock> hasher_;
  std::unique_ptr<testutil::TimerMock> timer_mock_;
  testutil::TimerMock *timer_;

  std::shared_ptr<BabeImpl> babe_;

  Epoch epoch_;
  BabeDuration slot_duration_{60ms};

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
  EXPECT_CALL(
      *lottery_,
      slotsLeadership(
          epoch_.randomness, A<Threshold>(), epoch_.epoch_duration, keypair_))
      .Times(2)
      .WillRepeatedly(Return(leadership_));

  // runSlot (3 times)
  EXPECT_CALL(*clock_, now())
      .WillOnce(Return(test_begin))
      .WillOnce(Return(test_begin + 60ms))
      .WillOnce(Return(test_begin + 60ms))
      .WillOnce(Return(test_begin + 120ms))
      .WillOnce(Return(test_begin + 120ms));

  EXPECT_CALL(*timer_, expiresAt(test_begin + slot_duration_));
  EXPECT_CALL(*timer_, expiresAt(test_begin + slot_duration_ * 2));
  EXPECT_CALL(*timer_, expiresAt(test_begin + slot_duration_ * 3));

  EXPECT_CALL(*timer_, asyncWait(_))
      .WillOnce(testing::InvokeArgument<0>(boost::system::error_code{}))
      .WillOnce(testing::InvokeArgument<0>(boost::system::error_code{}))
      .WillOnce({})
      .RetiresOnSaturation();

  // processSlotLeadership
  // we are not leader of the first slot, but leader of the second
  EXPECT_CALL(*block_tree_, deepestLeaf()).WillOnce(Return(best_leaf));
  EXPECT_CALL(*proposer_, propose(BlockId{best_block_hash_}, _, _))
      .WillOnce(Return(created_block_));
  EXPECT_CALL(*hasher_, blake2b_256(_)).WillOnce(Return(created_block_hash_));
  EXPECT_CALL(*block_tree_, addBlock(_)).WillOnce(Return(outcome::success()));

  EXPECT_CALL(*gossiper_, blockAnnounce(_))
      .WillOnce(CheckBlockHeader(created_block_.header));

  babe_->runEpoch(epoch_, test_begin + slot_duration_);
}
