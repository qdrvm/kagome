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
   * LevelDB
   */
  class InMemoryStorage : public storage::BufferStorage {
   public:
    ~InMemoryStorage() override = default;

    outcome::result<common::Buffer> load(
        const common::BufferView &key) const override;

    outcome::result<std::optional<common::Buffer>> tryLoad(
        const common::BufferView &key) const override;

    outcome::result<void> put(const common::BufferView &key,
                              const common::Buffer &value) override;

    outcome::result<void> put(const common::BufferView &key,
                              common::Buffer &&value) override;

    outcome::result<bool> contains(
        const common::BufferView &key) const override;

    bool empty() const override;

    outcome::result<void> remove(const common::BufferView &key) override;

    std::unique_ptr<
        kagome::storage::face::WriteBatch<common::BufferView, common::Buffer>>
    batch() override;

    std::unique_ptr<storage::BufferStorage::Cursor> cursor() override;

   private:
    std::map<std::string, common::Buffer> storage;
  };

}  // namespace kagome::storage

#endif  // KAGOME_STORAGE_IN_MEMORY_IN_MEMORY_STORAGE_HPP
