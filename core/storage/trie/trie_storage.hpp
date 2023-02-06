/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_TRIE_STORAGE
#define KAGOME_STORAGE_TRIE_TRIE_STORAGE

#include "common/blob.hpp"
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
    using OnDbRead = std::function<void(common::BufferView)>;

    virtual ~TrieStorage() = default;

    /**
     * Initializes a batch at the provided state
     * @warning if the batch is committed, the trie will still switch to its
     * state, creating a 'fork'
     */
    virtual outcome::result<std::unique_ptr<PersistentTrieBatch>>
    getPersistentBatchAt(const RootHash &root, OnDbRead on_db_read) = 0;
    virtual outcome::result<std::unique_ptr<EphemeralTrieBatch>>
    getEphemeralBatchAt(const RootHash &root, OnDbRead on_db_read) const = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_TRIE_STORAGE
