/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace kagome::parachain {
  // Define the scheduling lookahead constant to match Polkadot SDK's default
  // This is the number of blocks ahead/behind in the relay chain that we look for scheduling parachain blocks
  constexpr uint32_t DEFAULT_SCHEDULING_LOOKAHEAD = 3;
  
  // Legacy value for the minimum backing votes required
  constexpr uint32_t LEGACY_MIN_BACKING_VOTES = 2;
}  // namespace kagome::parachain 