/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <unordered_map>

#include "common/blob.hpp"
#include "outcome/outcome.hpp"
#include "primitives/kill_storage_result.hpp"
#include "storage/changes_trie/changes_tracker.hpp"
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
    using StateVersion = storage::trie::StateVersion;

    virtual ~TrieStorageProvider() = default;

    /**
     * Sets the current batch to a new ephemeral batch
     */
    virtual outcome::result<void> setToEphemeralAt(
        const common::Hash256 &state_root) = 0;

    /**
     * Sets the current batch to a new persistent batch at specified storage
     * state
     * @warning this will reset storage state and discard
     * all changes accumulated in the current batch
     */
    virtual outcome::result<void> setToPersistentAt(
        const common::Hash256 &state_root,
        TrieChangesTrackerOpt changes_tracker) = 0;

    /**
     * Sets the current batch to a new batch
     * @warning this will reset storage state to the specified root and discard
     * all changes accumulated in the current batch
     */
    virtual void setTo(std::shared_ptr<storage::trie::TrieBatch> batch) = 0;

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
    virtual outcome::result<
        std::reference_wrapper<const storage::trie::TrieBatch>>
    getChildBatchAt(const common::Buffer &root_path) = 0;

    virtual outcome::result<std::reference_wrapper<storage::trie::TrieBatch>>
    getMutableChildBatchAt(const common::Buffer &root_path) = 0;

    /**
     * Commits pending changes and returns the resulting state root
     * May or may not actually write to database depending
     * on the current batch type (persistent or ephemeral)
     */
    virtual outcome::result<storage::trie::RootHash> commit(
        const std::optional<BufferView> &child, StateVersion version) = 0;

    // ------ Transaction methods ------

    /// Start nested transaction
    virtual outcome::result<void> startTransaction() = 0;

    /// Rollback and finish last started transaction
    virtual outcome::result<void> rollbackTransaction() = 0;

    /// Commit and finish last started transaction
    virtual outcome::result<void> commitTransaction() = 0;

    // https://github.com/paritytech/polkadot-sdk/blob/c973fe86f8c668462186c95655a58fda04508e9a/substrate/primitives/state-machine/src/ext.rs#L438
    virtual KillStorageResult clearPrefix(
        const std::optional<BufferView> &child,
        BufferView prefix,
        const ClearPrefixLimit &limit) = 0;
  };

}  // namespace kagome::runtime
