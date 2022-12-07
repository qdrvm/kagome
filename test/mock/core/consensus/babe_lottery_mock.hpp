/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_LOTTERY_MOCK_HPP
#define KAGOME_BABE_LOTTERY_MOCK_HPP

#include "consensus/babe/babe_lottery.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::babe {
  struct BabeLotteryMock : public BabeLottery {
    MOCK_METHOD(void,
                changeEpoch,
                (const EpochDescriptor &,
                 const Randomness &,
                 const Threshold &,
                 const crypto::Sr25519Keypair &),
                (override));

    MOCK_METHOD(EpochDescriptor, getEpoch, (), (const, override));

    MOCK_METHOD(std::optional<crypto::VRFOutput>,
                getSlotLeadership,
                (primitives::BabeSlotNumber),
                (const, override));

    MOCK_METHOD(crypto::VRFOutput,
                slotVrfSignature,
                (primitives::BabeSlotNumber),
                (const, override));

    MOCK_METHOD(std::optional<primitives::AuthorityIndex>,
                secondarySlotAuthor,
                (primitives::BabeSlotNumber,
                 primitives::AuthorityListSize,
                 const Randomness &),
                (const, override));
  };
}  // namespace kagome::consensus::babe

#endif  // KAGOME_BABE_LOTTERY_MOCK_HPP
