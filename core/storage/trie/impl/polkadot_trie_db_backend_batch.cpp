/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
