/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABEUTILMOCK
#define KAGOME_CONSENSUS_BABEUTILMOCK

#include "consensus/babe/babe_util.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {

  class BabeUtilMock : public BabeUtil {
   public:
    using SyncFunctor = std::function<std::tuple<BabeSlotNumber, bool>()>;

    MOCK_METHOD(BabeSlotNumber, syncEpoch, (SyncFunctor &), ());

    BabeSlotNumber syncEpoch(SyncFunctor &&func) override {
      return syncEpoch(func);
    }

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

    MOCK_METHOD(BabeDuration, slotDuration, (), (const, override));

    MOCK_METHOD(EpochNumber, slotToEpoch, (BabeSlotNumber), (const, override));

    MOCK_METHOD(BabeSlotNumber,
                slotInEpoch,
                (BabeSlotNumber),
                (const, override));
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_BABEUTILMOCK
