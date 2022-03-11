/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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

  outcome::result<std::optional<common::Buffer>> InMemoryStorage::tryGet(
      const common::Buffer &key) const {
    if (storage.find(key.toHex()) != storage.end()) {
      return storage.at(key.toHex());
    }

    return std::nullopt;
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

  outcome::result<bool> InMemoryStorage::contains(const Buffer &key) const {
    return storage.find(key.toHex()) != storage.end();
  }

  bool InMemoryStorage::empty() const {
    return storage.empty();
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
