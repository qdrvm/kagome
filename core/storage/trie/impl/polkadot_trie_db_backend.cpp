/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/polkadot_trie_db_backend.hpp"

#include "storage/trie/impl/polkadot_trie_db_backend_batch.hpp"

namespace kagome::storage::trie {

  PolkadotTrieDbBackend::PolkadotTrieDbBackend(
      std::shared_ptr<PersistentBufferMap> storage,
      common::Buffer node_prefix,
      common::Buffer root_hash_key)
      : storage_{std::move(storage)},
        node_prefix_{std::move(node_prefix)},
        root_hash_key_{std::move(root_hash_key)} {
    BOOST_ASSERT(storage_ != nullptr);
  }

  outcome::result<void> PolkadotTrieDbBackend::saveRootHash(
      const common::Buffer &h) {
    return storage_->put(root_hash_key_, h);
  }

  outcome::result<common::Buffer> PolkadotTrieDbBackend::getRootHash() const {
    return storage_->get(root_hash_key_);
  }
  std::unique_ptr<face::MapCursor<Buffer, Buffer>>
  PolkadotTrieDbBackend::cursor() {
    return storage_
        ->cursor();  // TODO(Harrm): perhaps should iterate over trie nodes only
  }

  std::unique_ptr<face::WriteBatch<Buffer, Buffer>>
  PolkadotTrieDbBackend::batch() {
    return std::make_unique<PolkadotTrieDbBackendBatch>(storage_->batch(),
                                                        node_prefix_);
  }

  outcome::result<Buffer> PolkadotTrieDbBackend::get(const Buffer &key) const {
    return storage_->get(prefixKey(key));
  }

  bool PolkadotTrieDbBackend::contains(const Buffer &key) const {
    return storage_->contains(prefixKey(key));
  }

  outcome::result<void> PolkadotTrieDbBackend::put(const Buffer &key,
                                                   const Buffer &value) {
    return storage_->put(prefixKey(key), value);
  }

  outcome::result<void> PolkadotTrieDbBackend::put(const Buffer &key,
                                                   Buffer &&value) {
    return storage_->put(prefixKey(key), std::move(value));
  }

  outcome::result<void> PolkadotTrieDbBackend::remove(const Buffer &key) {
    return storage_->remove(prefixKey(key));
  }

  common::Buffer PolkadotTrieDbBackend::prefixKey(
      const common::Buffer &key) const {
    auto prefixed_key = node_prefix_;
    prefixed_key.put(key);
    return prefixed_key;
  }

}  // namespace kagome::storage::trie
