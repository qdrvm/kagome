/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/babe/babe_lottery.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::babe {
  struct BabeLotteryMock : public BabeLottery {
    MOCK_METHOD(EpochNumber, getEpoch, (), (const, override));

    MOCK_METHOD(bool,
                changeEpoch,
                (EpochNumber epoch, const primitives::BlockInfo &best_block),
                (override));

    MOCK_METHOD(std::optional<SlotLeadership>,
                getSlotLeadership,
                (const primitives::BlockHash &block, SlotNumber slot),
                (const, override));
  };
}  // namespace kagome::consensus::babe
