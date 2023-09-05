/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/slots_util.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {

  class SlotsUtilMock : public SlotsUtil {
   public:
    MOCK_METHOD(Duration, slotDuration, (), (const, override));

    MOCK_METHOD(EpochLength, epochLength, (), (const, override));

    MOCK_METHOD(SlotNumber, timeToSlot, (TimePoint), (const, override));

    MOCK_METHOD(TimePoint, slotStartTime, (SlotNumber slot), (const, override));

    MOCK_METHOD(TimePoint,
                slotFinishTime,
                (SlotNumber slot),
                (const, override));

    MOCK_METHOD(outcome::result<EpochNumber>,
                slotToEpoch,
                (const primitives::BlockInfo &, SlotNumber),
                (const));
  };

}  // namespace kagome::consensus
