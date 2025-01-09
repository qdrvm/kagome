/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "storage/spaced_storage.hpp"
#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::trie {

  class TrieStorageBackendImpl : public TrieStorageBackend {
   public:
    TrieStorageBackendImpl(std::shared_ptr<SpacedStorage> storage);

    ~TrieStorageBackendImpl() override = default;

    std::unique_ptr<Cursor> cursor() override;
    std::unique_ptr<BufferBatch> batch() override;

    outcome::result<BufferOrView> get(const BufferView &key) const override;
    outcome::result<std::optional<BufferOrView>> tryGet(
        const BufferView &key) const override;
    outcome::result<bool> contains(const BufferView &key) const override;

    outcome::result<void> put(const BufferView &key,
                              BufferOrView &&value) override;
    outcome::result<void> remove(const common::BufferView &key) override;

   private:
    std::shared_ptr<BufferStorage> storage_;
  };

}  // namespace kagome::storage::trie
