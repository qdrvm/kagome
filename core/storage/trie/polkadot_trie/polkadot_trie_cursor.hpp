/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/buffer_map_types.hpp"

#include "common/buffer.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"

namespace kagome::storage::trie {

  class PolkadotTrieCursor : public BufferStorageCursor {
   public:
    /**
     * Seek the first element with key not less than \arg key
     * @return true if the trie is not empty
     */
    virtual outcome::result<void> seekLowerBound(
        const common::BufferView &key) = 0;

    /**
     * Seek the first element with key greater than \arg key
     * @return true if the trie is not empty
     */
    virtual outcome::result<void> seekUpperBound(
        const common::BufferView &key) = 0;

    // small values (less than hash size) are not hashed and stored as-is inside
    // their node
    struct ValueHash {
      Hash256 hash;
      bool small;
    };
    [[nodiscard]] virtual std::optional<ValueHash> valueHash() const = 0;
  };

}  // namespace kagome::storage::trie
