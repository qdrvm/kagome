/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPERMOCK
#define KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPERMOCK

#include "consensus/babe/consistency_keeper.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::babe {

  class ConsistencyKeeperMock : public ConsistencyKeeper {
   public:
    MOCK_METHOD(Guard, start, (primitives::BlockInfo block), (override));
    MOCK_METHOD(void, commit, (primitives::BlockInfo block), (override));
    MOCK_METHOD(void, rollback, (primitives::BlockInfo block), (override));
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPERMOCK
