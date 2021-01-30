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
    MOCK_CONST_METHOD4(slotsLeadership,
                       SlotsLeadership(const EpochDescriptor &,
                                       const Randomness &,
                                       const Threshold &,
                                       const crypto::Sr25519Keypair &));

    MOCK_METHOD2(computeRandomness,
                 Randomness(const Randomness &, EpochNumber));

    MOCK_METHOD1(submitVRFValue, void(const crypto::VRFPreOutput &));
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_LOTTERY_MOCK_HPP
