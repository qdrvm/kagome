/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/trie_db_backend_impl.hpp"

#include <utility>

#include "storage/trie/impl/polkadot_trie_db_backend_batch.hpp"

namespace kagome::storage::trie {

  TrieDbBackendImpl::TrieDbBackendImpl(
      std::shared_ptr<PersistentBufferMap> storage, common::Buffer node_prefix)
      : storage_{std::move(storage)}, node_prefix_{std::move(node_prefix)} {
    BOOST_ASSERT(storage_ != nullptr);
  }

  std::unique_ptr<face::MapCursor<Buffer, Buffer>> TrieDbBackendImpl::cursor() {
    return storage_
        ->cursor();  // TODO(Harrm): perhaps should iterate over trie nodes only
  }

  std::unique_ptr<face::WriteBatch<Buffer, Buffer>> TrieDbBackendImpl::batch() {
    return std::make_unique<PolkadotTrieDbBackendBatch>(storage_->batch(),
                                                        node_prefix_);
  }

  outcome::result<Buffer> TrieDbBackendImpl::get(const Buffer &key) const {
    return storage_->get(prefixKey(key));
  }

  bool TrieDbBackendImpl::contains(const Buffer &key) const {
    return storage_->contains(prefixKey(key));
  }

  outcome::result<void> TrieDbBackendImpl::put(const Buffer &key,
                                               const Buffer &value) {
    return storage_->put(prefixKey(key), value);
  }

  outcome::result<void> TrieDbBackendImpl::put(const Buffer &key,
                                               Buffer &&value) {
    return storage_->put(prefixKey(key), std::move(value));
  }

  outcome::result<void> TrieDbBackendImpl::remove(const Buffer &key) {
    return storage_->remove(prefixKey(key));
  }

  common::Buffer TrieDbBackendImpl::prefixKey(const common::Buffer &key) const {
    return common::Buffer{node_prefix_}.put(key);
  }

}  // namespace kagome::storage::trie
