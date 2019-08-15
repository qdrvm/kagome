/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_IN_MEMORY_IN_MEMORY_STORAGE_HPP
#define KAGOME_STORAGE_IN_MEMORY_IN_MEMORY_STORAGE_HPP

#include <memory>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "storage/face/persistent_map.hpp"

namespace kagome::storage {

  using kagome::common::Buffer;
  
  /**
   * Simple storage that conforms PersistentMap interface
   * Mostly needed to have an in-memory trie in tests to avoid integration with
   * LevelDB
   */
  class InMemoryStorage
 : public kagome::storage::face::PersistentMap<Buffer, Buffer> {
   public:
    ~InMemoryStorage() override = default;

    outcome::result<Buffer> get(
        const Buffer &key) const override;

    outcome::result<void> put(const Buffer &key,
                              const Buffer &value) override;

    outcome::result<void> put(const Buffer &key,
                              Buffer &&value) override;

    bool contains(const Buffer &key) const override;

    outcome::result<void> remove(const Buffer &key) override;

    std::unique_ptr<
        kagome::storage::face::WriteBatch<Buffer, Buffer>>
    batch() override;

    std::unique_ptr<
        kagome::storage::face::MapCursor<Buffer, Buffer>>
    cursor() override;

   private:
    std::map<std::string, Buffer> storage;
  };

}  // namespace kagome::storage

#endif  // KAGOME_STORAGE_IN_MEMORY_IN_MEMORY_STORAGE_HPP
