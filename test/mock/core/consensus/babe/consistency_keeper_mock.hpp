/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPERMOCK
#define KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPERMOCK

#include "consensus/timeline/consistency_keeper.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::babe {

  class ConsistencyKeeperMock : public ConsistencyKeeper {
   public:
    MOCK_METHOD(ConsistencyGuard,
                start,
                (const primitives::BlockInfo &block),
                (override));
    MOCK_METHOD(void, commit, (const primitives::BlockInfo &block), (override));
    MOCK_METHOD(void,
                rollback,
                (const primitives::BlockInfo &block),
                (override));
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPERMOCK
