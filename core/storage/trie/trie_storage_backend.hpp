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
   * Provides storage for trie nodes and values.
   */
  class TrieStorageBackend {
   public:
    virtual ~TrieStorageBackend() = default;

    virtual BufferStorage &nodes() = 0;
    virtual BufferStorage &values() = 0;
    virtual std::unique_ptr<BufferSpacedBatch> batch() = 0;
  };

}  // namespace kagome::storage::trie
