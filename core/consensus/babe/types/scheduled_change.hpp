/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/unused.hpp"
#include "consensus/babe/types/babe_configuration.hpp"

namespace kagome::consensus::babe {

  struct NextConfigDataV1 final {
    std::pair<uint64_t, uint64_t> ratio;
    AllowedSlots second_slot{};
    bool operator==(const NextConfigDataV1 &other) const = default;
  };
  using NextConfigData = std::variant<Unused<0>, NextConfigDataV1>;

  struct OnDisabled {
    AuthorityIndex authority_index = 0;
  };

}  // namespace kagome::consensus::babe
