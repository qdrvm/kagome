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

#ifndef KAGOME_IN_MEMORY_BATCH_HPP
#define KAGOME_IN_MEMORY_BATCH_HPP

#include "common/buffer.hpp"
#include "storage/in_memory/in_memory_storage.hpp"

namespace kagome::storage {
  using kagome::common::Buffer;

  class InMemoryBatch
      : public kagome::storage::face::WriteBatch<Buffer,
                                                 Buffer> {
   public:
    explicit InMemoryBatch(InMemoryStorage &db) : db{db} {}

    outcome::result<void> put(const Buffer &key,
                              const Buffer &value) override {
      entries[key.toHex()] = value;
      return outcome::success();
    }

    outcome::result<void> put(const Buffer &key,
                              Buffer &&value) override {
      entries[key.toHex()] = std::move(value);
      return outcome::success();
    }

    outcome::result<void> remove(const Buffer &key) override {
      entries.erase(key.toHex());
      return outcome::success();
    }

    outcome::result<void> commit() override {
      for (auto &entry : entries) {
        OUTCOME_TRY(
            db.put(Buffer::fromHex(entry.first).value(), entry.second));
      }
      return outcome::success();
    }

    void clear() override {
      entries.clear();
    }

   private:
    std::map<std::string, Buffer> entries;
    InMemoryStorage &db;
  };
}  // namespace kagome::storage

#endif  // KAGOME_IN_MEMORY_BATCH_HPP
