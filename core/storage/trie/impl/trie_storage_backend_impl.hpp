/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_BACKEND
#define KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_BACKEND

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::trie {

  class TrieStorageBackendImpl : public TrieStorageBackend {
   public:
    TrieStorageBackendImpl(std::shared_ptr<BufferStorage> storage,
                           common::Buffer node_prefix);

    ~TrieStorageBackendImpl() override = default;

    std::unique_ptr<face::MapCursor<Buffer, Buffer>> cursor() override;
    std::unique_ptr<face::WriteBatch<Buffer, Buffer>> batch() override;

    outcome::result<Buffer> get(const Buffer &key) const override;
    outcome::result<std::optional<Buffer>> tryGet(
        const Buffer &key) const override;
    outcome::result<bool> contains(const Buffer &key) const override;
    bool empty() const override;

    outcome::result<void> put(const Buffer &key, const Buffer &value) override;
    outcome::result<void> put(const Buffer &key, Buffer &&value) override;
    outcome::result<void> remove(const Buffer &key) override;

   private:
    common::Buffer prefixKey(const common::Buffer &key) const;

    std::shared_ptr<BufferStorage> storage_;
    common::Buffer node_prefix_;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_BACKEND
