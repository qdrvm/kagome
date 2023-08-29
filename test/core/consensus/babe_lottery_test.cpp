/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/buffer.hpp"
#include "common/int_serialization.hpp"
#include "consensus/babe/impl/babe_lottery_impl.hpp"
#include "consensus/validation/prepare_transcript.hpp"
#include "mock/core/consensus/babe/babe_config_repository_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/crypto/vrf_provider_mock.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using namespace crypto;
using namespace consensus;
using namespace babe;
using namespace common;
using namespace primitives;

using testing::Return;

namespace vrf_constants = kagome::crypto::constants::sr25519::vrf;

struct BabeLotteryTest : public testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    keypair_.public_key.fill(2);
    keypair_.secret_key.fill(3);
  }

  std::shared_ptr<VRFProviderMock> vrf_provider_ =
      std::make_shared<VRFProviderMock>();

  BabeConfiguration babe_config_{
      .epoch_length = 3,
      .leadership_rate = {},
      .authorities = {},
      .randomness = {},
      .allowed_slots = {},
  };

  std::shared_ptr<BabeConfigRepositoryMock> babe_config_repo_ =
      std::make_shared<BabeConfigRepositoryMock>();

  std::shared_ptr<HasherMock> hasher_ = std::make_shared<HasherMock>();

  BabeLotteryImpl lottery_{vrf_provider_, babe_config_repo_, hasher_};

  EpochDescriptor current_epoch_;

  Randomness randomness_{{0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x44,
                          0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x44,
                          0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x44,
                          0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x44}};

  Sr25519Keypair keypair_{};

  Threshold threshold_{10};
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
  vrf_outputs.push_back({uint256_to_le_bytes(3749373), {}});
  vrf_outputs.push_back({uint256_to_le_bytes(1057472095), {}});

  for (size_t i = 0; i < babe_config_.epoch_length; ++i) {
    primitives::Transcript transcript;
    prepareTranscript(transcript, randomness_, i, current_epoch_.epoch_number);

    if (i == 2) {
      // just random case for testing
      EXPECT_CALL(*vrf_provider_,
                  signTranscript(transcript, keypair_, threshold_))
          .WillOnce(Return(std::nullopt));
      continue;
    }
    EXPECT_CALL(*vrf_provider_,
                signTranscript(transcript, keypair_, threshold_))
        .WillOnce(Return(vrf_outputs[i]));
  }

  // WHEN
  lottery_.changeEpoch(
      current_epoch_.epoch_number, randomness_, threshold_, keypair_);

  std::array<std::optional<VRFOutput>, 3> leadership = {
      lottery_.getSlotLeadership(0),
      lottery_.getSlotLeadership(1),
      lottery_.getSlotLeadership(2)};

  // THEN
  ASSERT_TRUE(leadership[0]);
  EXPECT_EQ(leadership[0]->output, uint256_to_le_bytes(3749373));
  ASSERT_TRUE(leadership[1]);
  EXPECT_EQ(leadership[1]->output, uint256_to_le_bytes(1057472095));
  ASSERT_FALSE(leadership[2]);
}
