/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/trie_storage_backend_batch.hpp"

namespace kagome::storage::trie {

  TrieStorageBackendBatch::TrieStorageBackendBatch(
      std::unique_ptr<BufferBatch> storage_batch)
      : storage_batch_{std::move(storage_batch)} {
    BOOST_ASSERT(storage_batch_ != nullptr);
  }

  outcome::result<void> TrieStorageBackendBatch::commit() {
    return storage_batch_->commit();
  }

  void TrieStorageBackendBatch::clear() {
    storage_batch_->clear();
  }

  outcome::result<void> TrieStorageBackendBatch::put(
      const common::BufferView &key, BufferOrView &&value) {
    return storage_batch_->put(key, std::move(value));
  }

  outcome::result<void> TrieStorageBackendBatch::remove(
      const common::BufferView &key) {
    return storage_batch_->remove(key);
  }

}  // namespace kagome::storage::trie
