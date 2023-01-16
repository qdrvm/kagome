/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_TRIE_STORAGE_PROVIDER
#define KAGOME_CORE_RUNTIME_TRIE_STORAGE_PROVIDER

#include <optional>
#include <unordered_map>

#include "common/blob.hpp"
#include "outcome/outcome.hpp"
#include "storage/trie/trie_batches.hpp"
#include "storage/trie/types.hpp"

namespace kagome::runtime {

  /**
   * Provides access to the trie storage for runtime API
   * As some calls need an access to a temporary storage (called 'ephemeral')
   * and some introduce changes that need to persist, TrieStorageProvider
   * maintains a 'current batch', which can be either persistent or ephemeral,
   * and provides it for runtime calls
   */
  class TrieStorageProvider {
   public:
    using Batch = storage::trie::TrieBatch;
    using PersistentBatch = storage::trie::PersistentTrieBatch;
    using StateVersion = storage::trie::StateVersion;
    using OnDbRead = std::function<void(common::BufferView)>;

    virtual ~TrieStorageProvider() = default;

    /**
     * Sets the current batch to a new ephemeral batch
     */
    virtual outcome::result<void> setToEphemeralAt(
        const common::Hash256 &state_root, OnDbRead on_db_read) = 0;

    /**
     * Sets the current batch to a new persistent batch at specified storage
     * state
     * @warning this will reset storage state to th specified root and discard
     * all changes accumulated in the current persistent batch
     */
    virtual outcome::result<void> setToPersistentAt(
        const common::Hash256 &state_root) = 0;

    /**
     * @returns current batch, if any was set (@see setToEphemeral,
     * setToPersistent), null otherwise
     */
    virtual std::shared_ptr<Batch> getCurrentBatch() const = 0;

    /**
     * @brief Get (or create new) Child Batch with given root hash
     *
     * @param root root hash value of a new (or cached) batch
     * @return Child storage tree batch
     */
    virtual outcome::result<std::shared_ptr<PersistentBatch>> getChildBatchAt(
        const common::Buffer &root_path) = 0;

    /**
     * Commits persistent changes even if the current batch is not persistent
     */
    virtual outcome::result<storage::trie::RootHash> forceCommit(
        StateVersion version) = 0;

    // ------ Transaction methods ------

    /// Start nested transaction
    virtual outcome::result<void> startTransaction() = 0;

    /// Rollback and finish last started transaction
    virtual outcome::result<void> rollbackTransaction() = 0;

    /// Commit and finish last started transaction
    virtual outcome::result<void> commitTransaction() = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_TRIE_STORAGE_PROVIDER
