/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/trie_storage_backend_impl.hpp"

#include <utility>

#include "storage/trie/impl/trie_storage_backend_batch.hpp"

namespace kagome::storage::trie {

  TrieStorageBackendImpl::TrieStorageBackendImpl(
      std::shared_ptr<BufferStorage> storage, common::Buffer node_prefix)
      : storage_{std::move(storage)}, node_prefix_{std::move(node_prefix)} {
    BOOST_ASSERT(storage_ != nullptr);
  }

  std::unique_ptr<TrieStorageBackendImpl::Cursor>
  TrieStorageBackendImpl::cursor() {
    return storage_
        ->cursor();  // TODO(Harrm): perhaps should iterate over trie nodes only
  }

  std::unique_ptr<BufferBatch> TrieStorageBackendImpl::batch() {
    return std::make_unique<TrieStorageBackendBatch>(storage_->batch(),
                                                     node_prefix_);
  }

  outcome::result<BufferOrView> TrieStorageBackendImpl::get(
      const BufferView &key) const {
    return storage_->get(prefixKey(key));
  }

  outcome::result<std::optional<BufferOrView>> TrieStorageBackendImpl::tryGet(
      const BufferView &key) const {
    return storage_->tryGet(prefixKey(key));
  }

  outcome::result<bool> TrieStorageBackendImpl::contains(
      const BufferView &key) const {
    return storage_->contains(prefixKey(key));
  }

  bool TrieStorageBackendImpl::empty() const {
    return storage_->empty();
  }

  outcome::result<void> TrieStorageBackendImpl::put(const BufferView &key,
                                                    BufferOrView &&value) {
    return storage_->put(prefixKey(key), std::move(value));
  }

  outcome::result<void> TrieStorageBackendImpl::remove(const BufferView &key) {
    return storage_->remove(prefixKey(key));
  }

  common::Buffer TrieStorageBackendImpl::prefixKey(
      const common::BufferView &key) const {
    return common::Buffer{node_prefix_}.put(key);
  }
}  // namespace kagome::storage::trie
