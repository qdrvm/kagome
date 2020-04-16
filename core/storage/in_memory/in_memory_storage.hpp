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

#ifndef KAGOME_STORAGE_IN_MEMORY_IN_MEMORY_STORAGE_HPP
#define KAGOME_STORAGE_IN_MEMORY_IN_MEMORY_STORAGE_HPP

#include <memory>

#include "outcome/outcome.hpp"
#include "common/buffer.hpp"
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

    outcome::result<common::Buffer> get(
        const common::Buffer &key) const override;

    outcome::result<void> put(const common::Buffer &key,
                              const common::Buffer &value) override;

    outcome::result<void> put(const common::Buffer &key,
                              common::Buffer &&value) override;

    bool contains(const common::Buffer &key) const override;

    outcome::result<void> remove(const common::Buffer &key) override;

    std::unique_ptr<
        kagome::storage::face::WriteBatch<common::Buffer, common::Buffer>>
    batch() override;

    std::unique_ptr<
        kagome::storage::face::MapCursor<common::Buffer, common::Buffer>>
    cursor() override;

   private:
    std::map<std::string, common::Buffer> storage;
  };

}  // namespace kagome::storage

#endif  // KAGOME_STORAGE_IN_MEMORY_IN_MEMORY_STORAGE_HPP
