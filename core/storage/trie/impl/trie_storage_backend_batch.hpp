/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/buffer_map_types.hpp"

namespace kagome::storage::trie {

  /**
   * Batch implementation for TrieStorageBackend
   * @see TrieStorageBackend
   */
  class TrieStorageBackendBatch : public BufferBatch {
   public:
    TrieStorageBackendBatch(std::unique_ptr<BufferBatch> storage_batch);
    ~TrieStorageBackendBatch() override = default;

    outcome::result<void> commit() override;

    outcome::result<void> put(const common::BufferView &key,
                              BufferOrView &&value) override;

    outcome::result<void> remove(const common::BufferView &key) override;
    void clear() override;

   private:
    std::unique_ptr<BufferBatch> storage_batch_;
  };

}  // namespace kagome::storage::trie
