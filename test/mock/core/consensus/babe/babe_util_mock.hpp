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
    MOCK_CONST_METHOD0(getCurrentSlot, BabeSlotNumber());

    MOCK_CONST_METHOD1(slotStartsIn, BabeDuration(BabeSlotNumber));

    MOCK_CONST_METHOD0(slotDuration, BabeDuration());

    MOCK_CONST_METHOD1(slotToEpoch, EpochNumber(BabeSlotNumber));

    MOCK_CONST_METHOD1(slotInEpoch, BabeSlotNumber(BabeSlotNumber));

    MOCK_METHOD1(setLastEpoch,
                 outcome::result<void>(const EpochDescriptor &led));

    MOCK_CONST_METHOD0(getLastEpoch, outcome::result<EpochDescriptor>());
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_BABEUTILMOCK
