/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "scale/big_fixed_integers.hpp"

namespace kagome::authority_discovery {
  using Timestamp = scale::uint128_t;
  using TimestampScale = scale::Fixed<Timestamp>;
}  // namespace kagome::authority_discovery
