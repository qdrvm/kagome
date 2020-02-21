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
#include "mock/core/consensus/babe_lottery_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/network/babe_gossiper_mock.hpp"
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
  std::shared_ptr<BabeLotteryMock> lottery_ =
      std::make_shared<BabeLotteryMock>();
  std::shared_ptr<ProposerMock> proposer_ = std::make_shared<ProposerMock>();
  std::shared_ptr<BlockTreeMock> block_tree_ =
      std::make_shared<BlockTreeMock>();
  std::shared_ptr<BabeGossiperMock> gossiper_ =
      std::make_shared<BabeGossiperMock>();
  SR25519Keypair keypair_{generateSR25519Keypair()};
  AuthorityIndex authority_id_ = {1};
  std::shared_ptr<SystemClockMock> clock_ = std::make_shared<SystemClockMock>();
  std::shared_ptr<HasherMock> hasher_ = std::make_shared<HasherMock>();
  std::unique_ptr<testutil::TimerMock> timer_mock_ =
      std::make_unique<testutil::TimerMock>();
  testutil::TimerMock *timer_ = timer_mock_.get();
  libp2p::event::Bus event_bus_;

  std::shared_ptr<BabeImpl> babe_ =
      std::make_shared<BabeImpl>(lottery_,
                                 proposer_,
                                 block_tree_,
                                 gossiper_,
                                 keypair_,
                                 authority_id_,
                                 clock_,
                                 hasher_,
                                 std::move(timer_mock_),
                                 event_bus_);

  Epoch epoch_{0, 0, 2, 60ms, {{}}, 100, {}};

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

  decltype(event_bus_.getChannel<event::BabeErrorChannel>()) &error_channel_{
      event_bus_.getChannel<event::BabeErrorChannel>()};
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
  EXPECT_CALL(*lottery_, slotsLeadership(epoch_, keypair_))
      .WillOnce(Return(leadership_));

  // runSlot (3 times)
  EXPECT_CALL(*clock_, now())
      .WillOnce(Return(test_begin))
      .WillOnce(Return(test_begin + 60ms))
      .WillOnce(Return(test_begin + 60ms))
      .WillOnce(Return(test_begin + 120ms));

  EXPECT_CALL(*timer_, expiresAt(test_begin + epoch_.slot_duration));
  EXPECT_CALL(*timer_, expiresAt(test_begin + epoch_.slot_duration * 2));
  EXPECT_CALL(*timer_, expiresAt(test_begin + epoch_.slot_duration * 3));

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

  EXPECT_CALL(*gossiper_, blockAnnounce(_))
      .WillOnce(CheckBlockHeader(created_block_.header));

  // finishEpoch
  auto new_epoch = epoch_;
  ++new_epoch.epoch_index;
  new_epoch.randomness.fill(5);
  EXPECT_CALL(*lottery_,
              computeRandomness(epoch_.randomness, new_epoch.epoch_index))
      .WillOnce(Return(new_epoch.randomness));

  // runEpoch
  EXPECT_CALL(*lottery_, slotsLeadership(new_epoch, keypair_))
      .WillOnce(Return(leadership_));

  babe_->runEpoch(epoch_, test_begin + epoch_.slot_duration);
}

/**
 * @given BABE production, which is configured to the already finished slot in
 * the current epoch
 * @when launching it
 * @then it synchronizes successfully
 */
TEST_F(BabeTest, SyncSuccess) {
  epoch_.epoch_duration = 10;
  epoch_.slot_duration = 5000ms;

  auto test_begin = real_clock_.now();
  auto delay = 9000ms;
  auto slot_start_time = test_begin - delay;

  // runEpoch
  epoch_.randomness.fill(0);
  EXPECT_CALL(*lottery_, slotsLeadership(epoch_, keypair_))
      .WillOnce(Return(leadership_));

  // runSlot
  // emulate relatively big delay
  EXPECT_CALL(*clock_, now())
      .WillOnce(Return(test_begin))
      .WillOnce(Return(test_begin + 50ms))
      .WillOnce(Return(test_begin + 100ms));

  // synchronizeSlots
  auto delay_in_slots =
      static_cast<uint64_t>(std::floor(delay / epoch_.slot_duration)) + 1;
  auto expected_current_slot = delay_in_slots + epoch_.start_slot - 1;
  auto expected_finish_slot_time =
      (delay_in_slots * epoch_.slot_duration) + slot_start_time;

  EXPECT_CALL(*timer_, expiresAt(slot_start_time + epoch_.slot_duration * 2));
  EXPECT_CALL(*timer_, asyncWait(_)).WillOnce({});

  babe_->runEpoch(epoch_, slot_start_time);

  auto meta = babe_->getBabeMeta();
  ASSERT_EQ(meta.current_slot_, expected_current_slot);
  ASSERT_EQ(meta.last_slot_finish_time_, expected_finish_slot_time);
}

/**
 * @given BABE production, which is configured to the already finished slot in
 * the previous epoch
 * @when launching it
 * @then it fails to synchronize
 */
TEST_F(BabeTest, BigDelay) {
  epoch_.epoch_duration = 1;

  auto test_begin = real_clock_.now();
  auto delay = 9000ms;
  auto slot_start_time = test_begin - delay;

  // runEpoch
  epoch_.randomness.fill(0);
  EXPECT_CALL(*lottery_, slotsLeadership(epoch_, keypair_))
      .WillOnce(Return(leadership_));

  // runSlot
  // emulate a very big delay (so that the system understands other nodes moved
  // to the next epoch already)
  EXPECT_CALL(*clock_, now())
      .Times(2)
      .WillRepeatedly(Return(
          test_begin + epoch_.slot_duration * epoch_.epoch_duration * 2));

  auto error_emitted = false;
  auto h = error_channel_.subscribe([&error_emitted](auto &&err) mutable {
    ASSERT_EQ(err, BabeError::NODE_FALL_BEHIND);
    error_emitted = true;
  });

  babe_->runEpoch(epoch_, slot_start_time);

  ASSERT_TRUE(error_emitted);
}
