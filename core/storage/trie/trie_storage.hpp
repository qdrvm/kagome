/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_TRIE_STORAGE
#define KAGOME_STORAGE_TRIE_TRIE_STORAGE

#include "common/blob.hpp"
#include "storage/changes_trie/changes_tracker.hpp"
#include "storage/trie/trie_batches.hpp"
#include "storage/trie/types.hpp"

namespace kagome::storage::trie {

  /**
   * Grants access to the storage in two ways:
   *  - persistent batch that will
   *    written back to the storage after commit() call
   *  - ephemeral batch, all changes to which are
   *    left in memory and thus the main storage is never changed by it
   */
  class TrieStorage {
   public:
    using EncodedNode = common::BufferView;
    using OnNodeLoaded =
        std::function<void(const common::Hash256 &, EncodedNode)>;

    virtual ~TrieStorage() = default;

    /**
     * Initializes a batch at the provided state
     * @warning if the batch is committed, the trie will still switch to its
     * state, creating a 'fork'
     */
    virtual outcome::result<std::unique_ptr<TrieBatch>> getPersistentBatchAt(
        const RootHash &root, TrieChangesTrackerOpt changes_tracker) = 0;

    virtual outcome::result<std::unique_ptr<TrieBatch>> getEphemeralBatchAt(
        const RootHash &root) const = 0;

    virtual outcome::result<std::unique_ptr<TrieBatch>> getProofReaderBatchAt(
        const RootHash &root, const OnNodeLoaded &on_node_loaded) const = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_TRIE_STORAGE
