/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace kagome::consensus::grandpa {
  enum class VoteType : uint8_t { Prevote, Precommit };
}
