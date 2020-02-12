/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_DB_BACKEND_HPP
#define KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_DB_BACKEND_HPP

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "storage/buffer_map.hpp"
#include "storage/trie/impl/polkadot_node.hpp"

namespace kagome::storage::trie {

  /**
   * Adapter for key-value storages that allows to hide keyspace separation
   * along with root hash storing logic from the trie db component
   */
  class PersistentTrieDbBackend : public PersistentBufferMap {
   public:
    PersistentTrieDbBackend(std::shared_ptr<PersistentBufferMap> storage,
                          common::Buffer node_prefix,
                          common::Buffer root_hash_key);

    ~PersistentTrieDbBackend() override = default;

    outcome::result<void> saveRootHash(const common::Buffer &h);
    outcome::result<common::Buffer> getRootHash() const;

    std::unique_ptr<face::MapCursor<Buffer, Buffer>> cursor() override;
    std::unique_ptr<face::WriteBatch<Buffer, Buffer>> batch() override;

    outcome::result<Buffer> get(const Buffer &key) const override;
    bool contains(const Buffer &key) const override;

    outcome::result<void> put(const Buffer &key, const Buffer &value) override;
    outcome::result<void> put(const Buffer &key, Buffer &&value) override;
    outcome::result<void> remove(const Buffer &key) override;

   private:
    common::Buffer prefixKey(const common::Buffer &key) const;

    std::shared_ptr<PersistentBufferMap> storage_;
    common::Buffer node_prefix_;
    common::Buffer root_hash_key_;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_DB_BACKEND_HPP
