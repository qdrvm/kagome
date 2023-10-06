/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRACKER
#define KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRACKER

#include "common/buffer.hpp"

namespace kagome::storage::changes_trie {

  /**
   * Stores the information about changes to the storage in accordance with the
   * supplied config. Used to build the changes trie.
   * onChange() must be called every time a persistent change to the node
   * storage is made.
   */
  class ChangesTracker {
   public:
    virtual ~ChangesTracker() = default;

    /**
     * Supposed to be called when an entry is put into the tracked storage
     * @arg new_entry states whether the entry is new, or just an update of a
     * present value
     */
    virtual void onPut(const common::BufferView &key,
                       const common::BufferView &value,
                       bool new_entry) = 0;

    /**
     * Supposed to be called when an entry is removed from the tracked storage
     */
    virtual void onRemove(const common::BufferView &key) = 0;
  };

}  // namespace kagome::storage::changes_trie

namespace kagome {
  using TrieChangesTrackerOpt =
      std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>;
}  // namespace kagome

#endif  // KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRACKER
