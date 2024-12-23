/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/trie_storage_backend_impl.hpp"

#include <utility>

#include "storage/spaces.hpp"
#include "storage/trie/impl/trie_storage_backend_batch.hpp"

namespace kagome::storage::trie {

  TrieStorageBackendImpl::TrieStorageBackendImpl(
      std::shared_ptr<SpacedStorage> storage)
      : storage_{storage->getSpace(Space::kTrieNode)} {
    BOOST_ASSERT(storage_ != nullptr);
  }

  std::unique_ptr<TrieStorageBackendImpl::Cursor>
  TrieStorageBackendImpl::cursor() {
    return storage_->cursor();
  }

  std::unique_ptr<BufferBatch> TrieStorageBackendImpl::batch() {
    return std::make_unique<TrieStorageBackendBatch>(storage_->batch());
  }

  outcome::result<BufferOrView> TrieStorageBackendImpl::get(
      const BufferView &key) const {
    return storage_->get(key);
  }

  outcome::result<std::optional<BufferOrView>> TrieStorageBackendImpl::tryGet(
      const BufferView &key) const {
    return storage_->tryGet(key);
  }

  outcome::result<bool> TrieStorageBackendImpl::contains(
      const BufferView &key) const {
    return storage_->contains(key);
  }

  outcome::result<void> TrieStorageBackendImpl::put(const BufferView &key,
                                                    BufferOrView &&value) {
    return storage_->put(key, std::move(value));
  }

  outcome::result<void> TrieStorageBackendImpl::remove(const BufferView &key) {
    return storage_->remove(key);
  }
}  // namespace kagome::storage::trie
