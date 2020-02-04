/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_DB_BACKEND_BATCH_HPP
#define KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_DB_BACKEND_BATCH_HPP

#include "common/buffer.hpp"
#include "storage/face/write_batch.hpp"

namespace kagome::storage::trie {

  /**
   * Batch implementation for PolkadotTrieDbBackend
   * @see PolkadotTrieDbBackend
   */
  class PolkadotTrieDbBackendBatch
      : public face::WriteBatch<common::Buffer, common::Buffer> {
   public:
    PolkadotTrieDbBackendBatch(
        std::unique_ptr<face::WriteBatch<common::Buffer, common::Buffer>>
            storage_batch,
        common::Buffer node_prefix);
    ~PolkadotTrieDbBackendBatch() override = default;

    outcome::result<void> commit() override;

    outcome::result<void> put(const common::Buffer &key,
                              const common::Buffer &value) override;

    outcome::result<void> put(const common::Buffer &key,
                              common::Buffer &&value) override;

    outcome::result<void> remove(const common::Buffer &key) override;
    void clear() override;

   private:
    common::Buffer prefixKey(const common::Buffer &key) const;

    std::unique_ptr<face::WriteBatch<common::Buffer, common::Buffer>>
        storage_batch_;
    common::Buffer node_prefix_;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_DB_BACKEND_BATCH_HPP
