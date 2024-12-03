/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "parachain/pvf/precheck.hpp"

namespace kagome::parachain {

  class PvfPrecheckMock : public IPvfPrecheck {
   public:
    MOCK_METHOD(void,
                start,
                (),
                (override));
  };

}  // namespace kagome::parachain
