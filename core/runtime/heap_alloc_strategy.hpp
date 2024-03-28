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
  /**
   * Allocate the initial heap pages as requested by the wasm file and then
   * allow it to grow dynamically.
   */
  struct HeapAllocStrategyDynamic {
    SCALE_TIE(1);
    SCALE_TIE_HASH_BOOST(HeapAllocStrategyDynamic);

    /**
     * The absolute maximum size of the linear memory (in pages).
     * When `Some(_)` the linear memory will be allowed to grow up to this
     * limit. When `None` the linear memory will be allowed to grow up to the
     * maximum limit supported by WASM (4GB).
     */
    std::optional<uint32_t> maximum_pages;
  };
  /**
   * Allocate a static number of heap pages.
   * The total number of allocated heap pages is the initial number of heap
   * pages requested by the wasm file plus the `extra_pages`.
   */
  struct HeapAllocStrategyStatic {
    SCALE_TIE(1);
    SCALE_TIE_HASH_BOOST(HeapAllocStrategyStatic);

    /**
     * The number of pages that will be added on top of the initial heap pages
     * requested by the wasm file.
     */
    uint32_t extra_pages;
  };
  /**
   * Defines the heap pages allocation strategy the wasm runtime should use.
   * A heap page is defined as 64KiB of memory.
   */
  using HeapAllocStrategy =
      boost::variant<HeapAllocStrategyDynamic, HeapAllocStrategyStatic>;
}  // namespace kagome
