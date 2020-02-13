/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_READONLY_TRIE_DB_BACKEND_HPP
#define KAGOME_READONLY_TRIE_DB_BACKEND_HPP

#include "storage/trie/trie_db_backend.hpp"

namespace kagome::storage::trie {

  /**
   * Constructs the Trie from the given root hash (assuming that no entries are
   * ever removed from the underlying key-value storage).
   * Prohibits any modification of the Trie (as its main purpose is to re-create
   * the storage state at a given block, it hardly makes sense to write into it)
   */
  class ReadonlyTrieDbBackend : public TrieDbBackend {
   public:
    enum Error {
      WRITE_TO_READONLY_TRIE = 1,
      CHANGE_ROOT_OF_READONLY_TRIE
    };

    ReadonlyTrieDbBackend(std::shared_ptr<PersistentBufferMap> storage,
                          common::Hash256 root_hash,
                          common::Buffer node_prefix);
    ~ReadonlyTrieDbBackend() override = default;

    std::unique_ptr<face::MapCursor<Buffer, Buffer>> cursor() override;

    outcome::result<Buffer> get(const Buffer &key) const override;
    bool contains(const Buffer &key) const override;

    outcome::result<Buffer> getRootHash() const override;

   private:
    outcome::result<void> saveRootHash(const common::Buffer &h) final;
    outcome::result<void> put(const Buffer &key, const Buffer &value) final;
    outcome::result<void> put(const Buffer &key, Buffer &&value) final;
    outcome::result<void> remove(const Buffer &key) final;
    std::unique_ptr<face::WriteBatch<Buffer, Buffer>> batch() final;

    std::shared_ptr<PersistentBufferMap> storage_;
    common::Buffer root_hash_;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, ReadonlyTrieDbBackend::Error);

#endif  // KAGOME_READONLY_TRIE_DB_BACKEND_HPP
