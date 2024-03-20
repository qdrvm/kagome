/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/heap_alloc_strategy.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/trie_batches.hpp"

namespace kagome {
  inline outcome::result<std::optional<HeapAllocStrategy>>
  heapAllocStrategyHeappages(const storage::trie::TrieBatch &trie) {
    OUTCOME_TRY(raw, trie.tryGet(storage::kRuntimeHeappagesKey));
    if (raw) {
      if (auto r = scale::decode<uint64_t>(*raw)) {
        return HeapAllocStrategyStatic{static_cast<uint32_t>(r.value())};
      }
    }
    return std::nullopt;
  }
}  // namespace kagome
