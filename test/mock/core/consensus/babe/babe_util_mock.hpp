/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/babe/babe_util.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::babe {

  class BabeUtilMock : public BabeUtil {
   public:
    MOCK_METHOD(SlotNumber, timeToSlot, (TimePoint), (const, override));

    MOCK_METHOD(TimePoint, slotStartTime, (SlotNumber slot), (const, override));

    MOCK_METHOD(TimePoint,
                slotFinishTime,
                (SlotNumber slot),
                (const, override));

    MOCK_METHOD(outcome::result<EpochDescriptor>,
                slotToEpochDescriptor,
                (const primitives::BlockInfo &, SlotNumber),
                (const, override));
  };

}  // namespace kagome::consensus::babe
