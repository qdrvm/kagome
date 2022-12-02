/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_BACKEND_BATCH
#define KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_BACKEND_BATCH

#include "storage/buffer_map_types.hpp"

namespace kagome::storage::trie {

  /**
   * Batch implementation for TrieStorageBackend
   * @see TrieStorageBackend
   */
  class TrieStorageBackendBatch
      : public face::WriteBatch<common::BufferView, common::Buffer> {
   public:
    TrieStorageBackendBatch(
        std::unique_ptr<face::WriteBatch<common::BufferView, common::Buffer>>
            storage_batch,
        common::Buffer node_prefix);
    ~TrieStorageBackendBatch() override = default;

    outcome::result<void> commit() override;

    outcome::result<void> put(const common::BufferView &key,
                              BufferOrView &&value) override;

    outcome::result<void> remove(const common::BufferView &key) override;
    void clear() override;

   private:
    common::Buffer prefixKey(const common::BufferView &key) const;

    std::unique_ptr<face::WriteBatch<common::BufferView, common::Buffer>>
        storage_batch_;
    common::Buffer node_prefix_;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_BACKEND_BATCH
