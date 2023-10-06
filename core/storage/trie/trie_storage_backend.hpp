/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>

#include "common/buffer.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"

namespace kagome::storage::trie {

  /**
   * Adapter for key-value storages that allows to hide keyspace separation
   * along with root hash storing logic from the trie db component
   */
  class TrieStorageBackend : public BufferStorage {
   public:
    ~TrieStorageBackend() override = default;
  };

}  // namespace kagome::storage::trie
