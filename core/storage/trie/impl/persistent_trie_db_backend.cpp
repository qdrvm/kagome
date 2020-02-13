/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/persistent_trie_db_backend.hpp"

#include <utility>

#include "storage/trie/impl/polkadot_trie_db_backend_batch.hpp"

namespace kagome::storage::trie {

  PersistentTrieDbBackend::PersistentTrieDbBackend(
      std::shared_ptr<PersistentBufferMap> storage,
      common::Buffer root_hash_key,
      common::Buffer node_prefix)
      : TrieDbBackend{std::move(node_prefix)},
        storage_{std::move(storage)},
        root_hash_key_{std::move(root_hash_key)} {
    BOOST_ASSERT(storage_ != nullptr);
  }

  outcome::result<void> PersistentTrieDbBackend::saveRootHash(
      const common::Buffer &h) {
    return storage_->put(root_hash_key_, h);
  }

  outcome::result<common::Buffer> PersistentTrieDbBackend::getRootHash() const {
    return storage_->get(root_hash_key_);
  }
  std::unique_ptr<face::MapCursor<Buffer, Buffer>>
  PersistentTrieDbBackend::cursor() {
    return storage_
        ->cursor();  // TODO(Harrm): perhaps should iterate over trie nodes only
  }

  std::unique_ptr<face::WriteBatch<Buffer, Buffer>>
  PersistentTrieDbBackend::batch() {
    return std::make_unique<PolkadotTrieDbBackendBatch>(storage_->batch(),
                                                        getNodePrefix());
  }

  outcome::result<Buffer> PersistentTrieDbBackend::get(
      const Buffer &key) const {
    return storage_->get(prefixKey(key));
  }

  bool PersistentTrieDbBackend::contains(const Buffer &key) const {
    return storage_->contains(prefixKey(key));
  }

  outcome::result<void> PersistentTrieDbBackend::put(const Buffer &key,
                                                     const Buffer &value) {
    return storage_->put(prefixKey(key), value);
  }

  outcome::result<void> PersistentTrieDbBackend::put(const Buffer &key,
                                                     Buffer &&value) {
    return storage_->put(prefixKey(key), std::move(value));
  }

  outcome::result<void> PersistentTrieDbBackend::remove(const Buffer &key) {
    return storage_->remove(prefixKey(key));
  }

}  // namespace kagome::storage::trie
