/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_TRIE_STORAGE_PROVIDER
#define KAGOME_CORE_RUNTIME_TRIE_STORAGE_PROVIDER

#include <boost/optional.hpp>

#include "outcome/outcome.hpp"
#include "common/blob.hpp"
#include "storage/trie/trie_batches.hpp"

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

    virtual ~TrieStorageProvider() = default;

    /**
     * Sets the current batch to a new ephemeral batch
     */
    virtual outcome::result<void> setToEphemeral() = 0;
    virtual outcome::result<void> setToEphemeralAt(
        const common::Hash256 &state_root) = 0;

    /**
     * Sets the current batch to the persistent batch (a persistent batch is
     * unique as it accumulates changes for commit)
     */
    virtual outcome::result<void> setToPersistent() = 0;

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
     * @returns current persistent batch, if the current batch is persistent,
     * none otherwise
     */
    virtual boost::optional<std::shared_ptr<PersistentBatch>>
    tryGetPersistentBatch() const = 0;

    /**
     * @returns true, if the current batch is persistent,
     * false otherwise
     */
    virtual bool isCurrentlyPersistent() const = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_TRIE_STORAGE_PROVIDER
