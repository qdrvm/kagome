/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
                (SlotNumber),
                (const, override));

    MOCK_METHOD(crypto::VRFOutput,
                slotVrfSignature,
                (SlotNumber),
                (const, override));

    MOCK_METHOD(std::optional<primitives::AuthorityIndex>,
                secondarySlotAuthor,
                (SlotNumber, primitives::AuthorityListSize, const Randomness &),
                (const, override));
  };
}  // namespace kagome::consensus::babe
