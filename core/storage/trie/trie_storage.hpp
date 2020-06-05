/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_TRIE_STORAGE
#define KAGOME_STORAGE_TRIE_TRIE_STORAGE

#include "common/blob.hpp"
#include "storage/trie/trie_batches.hpp"

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
    virtual ~TrieStorage() = default;

    virtual outcome::result<std::unique_ptr<PersistentTrieBatch>>
    getPersistentBatch() = 0;
    virtual outcome::result<std::unique_ptr<EphemeralTrieBatch>>
    getEphemeralBatch() const = 0;

    /**
     * Initializes a batch at the provided state
     * @warning if the batch is committed, the trie will still switch to its
     * state, creating a 'fork'
     */
    virtual outcome::result<std::unique_ptr<PersistentTrieBatch>>
    getPersistentBatchAt(const common::Hash256 &root) = 0;
    virtual outcome::result<std::unique_ptr<EphemeralTrieBatch>>
    getEphemeralBatchAt(const common::Hash256 &root) const = 0;

    /**
     * Root hash of the latest committed trie
     */
    virtual common::Buffer getRootHash() const = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_TRIE_STORAGE
