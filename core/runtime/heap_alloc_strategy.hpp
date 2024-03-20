/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>
#include <cstdint>
#include <optional>

#include "scale/tie.hpp"
#include "scale/tie_hash.hpp"

namespace kagome {
  struct HeapAllocStrategyDynamic {
    SCALE_TIE(1);
    SCALE_TIE_HASH_BOOST(HeapAllocStrategyDynamic);

    std::optional<uint32_t> maximum_pages;
  };
  struct HeapAllocStrategyStatic {
    SCALE_TIE(1);
    SCALE_TIE_HASH_BOOST(HeapAllocStrategyStatic);

    uint32_t extra_pages;
  };
  using HeapAllocStrategy =
      boost::variant<HeapAllocStrategyDynamic, HeapAllocStrategyStatic>;
}  // namespace kagome
