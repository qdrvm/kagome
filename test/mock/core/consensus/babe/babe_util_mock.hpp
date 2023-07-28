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
    MOCK_METHOD(BabeSlotNumber, getFirstBlockSlotNumber, (), ());

    MOCK_METHOD(BabeSlotNumber, getCurrentSlot, (), (const, override));

    MOCK_METHOD(BabeTimePoint,
                slotStartTime,
                (BabeSlotNumber slot),
                (const, override));

    MOCK_METHOD(BabeDuration,
                remainToStartOfSlot,
                (BabeSlotNumber slot),
                (const, override));

    MOCK_METHOD(BabeTimePoint,
                slotFinishTime,
                (BabeSlotNumber slot),
                (const, override));

    MOCK_METHOD(BabeDuration,
                remainToFinishOfSlot,
                (BabeSlotNumber slot),
                (const, override));

    MOCK_METHOD(EpochNumber, slotToEpoch, (BabeSlotNumber), (const, override));

    MOCK_METHOD(BabeSlotNumber,
                slotInEpoch,
                (BabeSlotNumber),
                (const, override));
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABEUTILMOCK
