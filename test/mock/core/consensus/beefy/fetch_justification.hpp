/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/beefy/fetch_justification.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::beefy {
  struct FetchJustificationMock : FetchJustification {
    MOCK_METHOD(void,
                fetchJustification,
                (primitives::BlockNumber),
                (override));
  };
}  // namespace kagome::consensus::beefy
