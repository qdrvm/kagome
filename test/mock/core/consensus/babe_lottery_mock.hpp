/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_LOTTERY_MOCK_HPP
#define KAGOME_BABE_LOTTERY_MOCK_HPP

#include "consensus/babe/babe_lottery.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {
  struct BabeLotteryMock : public BabeLottery {
    MOCK_METHOD4(changeEpoch,
                 void(const EpochDescriptor &,
                      const Randomness &,
                      const Threshold &,
                      const crypto::Sr25519Keypair &));

    MOCK_CONST_METHOD0(epoch, EpochDescriptor());

    MOCK_CONST_METHOD1(
        getSlotLeadership,
        boost::optional<crypto::VRFOutput>(primitives::BabeSlotNumber));

    MOCK_METHOD2(computeRandomness,
                 Randomness(const Randomness &, EpochNumber));

    MOCK_METHOD1(submitVRFValue, void(const crypto::VRFPreOutput &));
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_LOTTERY_MOCK_HPP
