/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABEUTILMOCK
#define KAGOME_CONSENSUS_BABEUTILMOCK

#include "consensus/babe/babe_util.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::babe {

  class BabeUtilMock : public BabeUtil {
   public:
    MOCK_METHOD(BabeSlotNumber, timeToSlot, (BabeTimePoint), (const, override));

    MOCK_METHOD(BabeTimePoint,
                slotStartTime,
                (BabeSlotNumber slot),
                (const, override));

    MOCK_METHOD(BabeTimePoint,
                slotFinishTime,
                (BabeSlotNumber slot),
                (const, override));

    MOCK_METHOD(outcome::result<EpochDescriptor>,
                slotToEpochDescriptor,
                (const primitives::BlockInfo &, BabeSlotNumber),
                (const, override));
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABEUTILMOCK
