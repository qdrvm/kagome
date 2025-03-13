/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/types/authority.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::grandpa {

  struct ScheduledChange {
    Authorities authorities{};
    uint32_t subchain_length = 0;
  };

  struct ForcedChange {
    primitives::BlockNumber delay_start = 0;
    Authorities authorities{};
    uint32_t subchain_length = 0;
  };

  struct OnDisabled {
    AuthorityIndex authority_index = 0;
  };

  struct Pause {
    uint32_t subchain_length = 0;
  };

  struct Resume {
    uint32_t subchain_length = 0;
  };

}  // namespace kagome::consensus::grandpa
