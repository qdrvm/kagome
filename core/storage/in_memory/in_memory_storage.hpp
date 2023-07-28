/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_IN_MEMORY_IN_MEMORY_STORAGE_HPP
#define KAGOME_STORAGE_IN_MEMORY_IN_MEMORY_STORAGE_HPP

#include <memory>

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::storage {

  /**
   * Simple storage that conforms PersistentMap interface
   * Mostly needed to have an in-memory trie in tests to avoid integration with
   * an actual persistent database
   */
  class InMemoryStorage : public storage::BufferStorage {
   public:
    ~InMemoryStorage() override = default;

    outcome::result<BufferOrView> get(
        const common::BufferView &key) const override;

    outcome::result<std::optional<BufferOrView>> tryGet(
        const common::BufferView &key) const override;

    outcome::result<void> put(const common::BufferView &key,
                              BufferOrView &&value) override;

    outcome::result<bool> contains(
        const common::BufferView &key) const override;

    bool empty() const override;

    outcome::result<void> remove(const common::BufferView &key) override;

    std::unique_ptr<BufferBatch> batch() override;

    std::unique_ptr<Cursor> cursor() override;

    std::optional<size_t> byteSizeHint() const override;

   private:
    std::map<std::string, common::Buffer> storage;
    size_t size_ = 0;

    friend class InMemoryCursor;
  };

}  // namespace kagome::storage

#endif  // KAGOME_STORAGE_IN_MEMORY_IN_MEMORY_STORAGE_HPP
