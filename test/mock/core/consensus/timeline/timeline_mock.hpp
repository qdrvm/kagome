/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/timeline.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {

  class TimelineMock final : public Timeline {
   public:
    MOCK_METHOD(SyncState, getCurrentState, (), (const, override));

    MOCK_METHOD(bool, wasSynchronized, (), (const, override));

    MOCK_METHOD(void,
                checkAndReportEquivocation,
                (const primitives::BlockHeader &),
                (override));
  };

}  // namespace kagome::consensus
