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

#include "storage/in_memory/in_memory_storage.hpp"

#include "storage/database_error.hpp"
#include "storage/in_memory/in_memory_batch.hpp"

using kagome::common::Buffer;

namespace kagome::storage {

  outcome::result<Buffer> InMemoryStorage::get(const Buffer &key) const {
    if (storage.find(key.toHex()) != storage.end()) {
      return storage.at(key.toHex());
    }

    return DatabaseError::NOT_FOUND;
  }

  outcome::result<void> InMemoryStorage::put(const Buffer &key,
                                             const Buffer &value) {
    storage[key.toHex()] = value;
    return outcome::success();
  }

  outcome::result<void> InMemoryStorage::put(const Buffer &key,
                                             Buffer &&value) {
    storage[key.toHex()] = std::move(value);
    return outcome::success();
  }

  bool InMemoryStorage::contains(const Buffer &key) const {
    return storage.find(key.toHex()) != storage.end();
  }

  outcome::result<void> InMemoryStorage::remove(const Buffer &key) {
    storage.erase(key.toHex());
    return outcome::success();
  }

  std::unique_ptr<kagome::storage::face::WriteBatch<Buffer, Buffer>>
  InMemoryStorage::batch() {
    return std::make_unique<InMemoryBatch>(*this);
  }

  std::unique_ptr<kagome::storage::face::MapCursor<Buffer, Buffer>>
  InMemoryStorage::cursor() {
    return nullptr;
  }
}  // namespace kagome::storage
