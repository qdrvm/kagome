/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/int_serialization.hpp"
#include "consensus/babe/impl/babe_lottery_impl.hpp"
#include "consensus/babe/impl/prepare_transcript.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "mock/core/consensus/babe/babe_config_repository_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/crypto/session_keys_mock.hpp"
#include "mock/core/crypto/vrf_provider_mock.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using namespace crypto;
using namespace consensus;
using namespace babe;
using namespace common;
using namespace primitives;

using testing::_;
using testing::Return;

namespace vrf_constants = kagome::crypto::constants::sr25519::vrf;

struct BabeLotteryTest : public testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    keypair_.public_key.fill(2);
    //keypair_.secret_key.fill(3);

    babe_config_ = std::make_shared<BabeConfiguration>(BabeConfiguration{
        .epoch_length = 3,
        .leadership_rate = {3, 4},
        .authorities = {{{keypair_.public_key}, 1}},
        .randomness =
            Randomness{{0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x44,
                        0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x44,
                        0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x44,
                        0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x44}},
        .allowed_slots = {},
    });

    ON_CALL(*babe_config_repo_, config(_, _))
        .WillByDefault(Return(babe_config_));

    ON_CALL(*session_keys_, getBabeKeyPair(_))
        .WillByDefault(Return(std::make_optional(
            std::pair(std::make_shared<Sr25519Keypair>(keypair_), 0))));

    threshold_ = kagome::consensus::babe::calculateThreshold(
        babe_config_->leadership_rate, babe_config_->authorities, 0);
  }

  std::shared_ptr<BabeConfiguration> babe_config_;

  std::shared_ptr<BabeConfigRepositoryMock> babe_config_repo_ =
      std::make_shared<BabeConfigRepositoryMock>();

  std::shared_ptr<SessionKeysMock> session_keys_ =
      std::make_shared<SessionKeysMock>();

  std::shared_ptr<VRFProviderMock> vrf_provider_ =
      std::make_shared<VRFProviderMock>();

  std::shared_ptr<HasherMock> hasher_ = std::make_shared<HasherMock>();

  BabeLotteryImpl lottery_{
      babe_config_repo_, session_keys_, vrf_provider_, hasher_};

  EpochNumber current_epoch_;

  Sr25519Keypair keypair_{};

  Threshold threshold_{};
};

/**
 * @given BabeLottery with a number of VRF values submitted
 * @when computing leadership for the epoch
 * @then leadership is computed as intended
 */
TEST_F(BabeLotteryTest, SlotsLeadership) {
  // GIVEN

  std::vector<VRFOutput> vrf_outputs;
  vrf_outputs.reserve(2);
  vrf_outputs.push_back({uint256_to_le_bytes(1234567), {}});
  vrf_outputs.push_back({uint256_to_le_bytes(7654321), {}});

  for (size_t slot = 0; slot < babe_config_->epoch_length; ++slot) {
    primitives::Transcript transcript;
    prepareTranscript(
        transcript, babe_config_->randomness, slot, current_epoch_);

    if (slot == 2) {
      // just random case for testing
      EXPECT_CALL(*vrf_provider_,
                  signTranscript(transcript, keypair_, threshold_))
          .WillOnce(Return(std::nullopt));
      continue;
    }
    EXPECT_CALL(*vrf_provider_,
                signTranscript(transcript, keypair_, threshold_))
        .WillOnce(Return(vrf_outputs[slot]));
  }

  // WHEN
  lottery_.changeEpoch(current_epoch_, {});

  std::array<std::optional<SlotLeadership>, 3> leadership = {
      lottery_.getSlotLeadership({}, 0),
      lottery_.getSlotLeadership({}, 1),
      lottery_.getSlotLeadership({}, 2)};

  // THEN
  ASSERT_TRUE(leadership[0]);
  EXPECT_EQ(leadership[0]->vrf_output.output, uint256_to_le_bytes(1234567));
  ASSERT_TRUE(leadership[1]);
  EXPECT_EQ(leadership[1]->vrf_output.output, uint256_to_le_bytes(7654321));
  ASSERT_FALSE(leadership[2]);
}
