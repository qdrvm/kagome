/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/polkadot_trie_db_backend_batch.hpp"

namespace kagome::storage::trie {

  PolkadotTrieDbBackendBatch::PolkadotTrieDbBackendBatch(
      std::unique_ptr<face::WriteBatch<common::Buffer, common::Buffer>>
          storage_batch,
      common::Buffer node_prefix)
      : storage_batch_{std::move(storage_batch)}, node_prefix_{std::move(node_prefix)} {
    BOOST_ASSERT(storage_batch_ != nullptr);
  }

  outcome::result<void> PolkadotTrieDbBackendBatch::commit() {
    return storage_batch_->commit();
  }

  void PolkadotTrieDbBackendBatch::clear() {
    storage_batch_->clear();
  }

  outcome::result<void> PolkadotTrieDbBackendBatch::put(
      const common::Buffer &key, const common::Buffer &value) {
    return storage_batch_->put(prefixKey(key), value);
  }

  outcome::result<void> PolkadotTrieDbBackendBatch::put(
      const common::Buffer &key, common::Buffer &&value) {
    return storage_batch_->put(prefixKey(key), std::move(value));
  }

  outcome::result<void> PolkadotTrieDbBackendBatch::remove(
      const common::Buffer &key) {
    return storage_batch_->remove(prefixKey(key));
  }

  common::Buffer PolkadotTrieDbBackendBatch::prefixKey(const common::Buffer &key) const {
    auto prefixed_key = node_prefix_;
    prefixed_key.put(key);
    return prefixed_key;
  }

}  // namespace kagome::storage::trie
