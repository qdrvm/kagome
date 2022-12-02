/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/in_memory/in_memory_storage.hpp"

#include "storage/database_error.hpp"
#include "storage/in_memory/in_memory_batch.hpp"

using kagome::common::Buffer;

namespace kagome::storage {

  outcome::result<common::Buffer> InMemoryStorage::load(
      const BufferView &key) const {
    if (storage.find(key.toHex()) != storage.end()) {
      return storage.at(key.toHex());
    }

    return DatabaseError::NOT_FOUND;
  }

  outcome::result<std::optional<Buffer>> InMemoryStorage::tryLoad(
      const common::BufferView &key) const {
    if (storage.find(key.toHex()) != storage.end()) {
      return storage.at(key.toHex());
    }

    return std::nullopt;
  }

  outcome::result<void> InMemoryStorage::put(const BufferView &key,
                                             BufferOrView &&value) {
    auto it = storage.find(key.toHex());
    if (it != storage.end()) {
      size_t old_value_size = it->second.size();
      BOOST_ASSERT(size_ >= old_value_size);
      size_ -= old_value_size;
    }
    size_ += value.size();
    storage[key.toHex()] = value.into();
    return outcome::success();
  }

  outcome::result<bool> InMemoryStorage::contains(const BufferView &key) const {
    return storage.find(key.toHex()) != storage.end();
  }

  bool InMemoryStorage::empty() const {
    return storage.empty();
  }

  outcome::result<void> InMemoryStorage::remove(const BufferView &key) {
    auto it = storage.find(key.toHex());
    if (it != storage.end()) {
      size_ -= it->second.size();
      storage.erase(it);
    }
    return outcome::success();
  }

  std::unique_ptr<kagome::storage::face::WriteBatch<BufferView, Buffer>>
  InMemoryStorage::batch() {
    return std::make_unique<InMemoryBatch>(*this);
  }

  std::unique_ptr<InMemoryStorage::Cursor> InMemoryStorage::cursor() {
    return nullptr;
  }

  size_t InMemoryStorage::size() const {
    return size_;
  }
}  // namespace kagome::storage
