/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/heap_alloc_strategy.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome {
  constexpr uint32_t kDefaultHeapAllocPages = 2048;

  /// Convert ":heappages" from state trie to `HeapAllocStrategy`.
  inline outcome::result<std::optional<HeapAllocStrategy>>
  heapAllocStrategyHeappages(const storage::BufferStorage &trie) {
    OUTCOME_TRY(raw, trie.tryGet(storage::kRuntimeHeappagesKey));
    if (raw) {
      if (auto r = scale::decode<uint64_t>(*raw)) {
        return HeapAllocStrategyStatic{static_cast<uint32_t>(r.value())};
      }
    }
    return std::nullopt;
  }

  inline outcome::result<HeapAllocStrategy> heapAllocStrategyHeappagesDefault(
      const storage::BufferStorage &trie) {
    OUTCOME_TRY(config, heapAllocStrategyHeappages(trie));
    return config.value_or(HeapAllocStrategyStatic{kDefaultHeapAllocPages});
  }
}  // namespace kagome
