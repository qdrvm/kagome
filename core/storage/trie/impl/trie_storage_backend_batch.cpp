/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/trie_storage_backend_batch.hpp"

namespace kagome::storage::trie {

  TrieStorageBackendBatch::TrieStorageBackendBatch(
      std::unique_ptr<face::WriteBatch<common::BufferView, common::Buffer>>
          storage_batch,
      common::Buffer node_prefix)
      : storage_batch_{std::move(storage_batch)},
        node_prefix_{std::move(node_prefix)} {
    BOOST_ASSERT(storage_batch_ != nullptr);
  }

  outcome::result<void> TrieStorageBackendBatch::commit() {
    return storage_batch_->commit();
  }

  void TrieStorageBackendBatch::clear() {
    storage_batch_->clear();
  }

  outcome::result<void> TrieStorageBackendBatch::put(
      const common::BufferView &key, const common::Buffer &value) {
    return storage_batch_->put(prefixKey(key), value);
  }

  outcome::result<void> TrieStorageBackendBatch::put(
      const common::BufferView &key, common::Buffer &&value) {
    return storage_batch_->put(prefixKey(key), std::move(value));
  }

  outcome::result<void> TrieStorageBackendBatch::remove(
      const common::BufferView &key) {
    return storage_batch_->remove(prefixKey(key));
  }

  common::Buffer TrieStorageBackendBatch::prefixKey(
      const common::BufferView &key) const {
    auto prefixed_key = node_prefix_;
    prefixed_key.put(key);
    return prefixed_key;
  }

}  // namespace kagome::storage::trie
