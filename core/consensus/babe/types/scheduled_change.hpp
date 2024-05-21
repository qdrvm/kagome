/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/unused.hpp"
#include "consensus/babe/types/authority.hpp"
#include "consensus/babe/types/babe_configuration.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus::babe {

  struct NextConfigDataV1 final {
    SCALE_TIE(2);
    std::pair<uint64_t, uint64_t> ratio;
    AllowedSlots second_slot;
  };
  using NextConfigData = std::variant<Unused<0>, NextConfigDataV1>;

  struct OnDisabled {
    SCALE_TIE(1);
    AuthorityIndex authority_index = 0;
  };

}  // namespace kagome::consensus::babe
