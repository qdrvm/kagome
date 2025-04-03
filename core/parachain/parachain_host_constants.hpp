/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace kagome::parachain {
  // Define the scheduling lookahead constant to match Polkadot SDK's default
  // This is the number of blocks ahead/behind in the relay chain that we look
  // for scheduling parachain blocks
  // https://github.com/paritytech/polkadot-sdk/blob/1d5d8aac07d41f3c2aa57a80562af7955dcc8af3/polkadot/primitives/src/v8/mod.rs#L460
  constexpr uint32_t DEFAULT_SCHEDULING_LOOKAHEAD = 3;
}  // namespace kagome::parachain
