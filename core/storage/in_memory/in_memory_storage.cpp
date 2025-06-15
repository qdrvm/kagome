/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/in_memory/in_memory_storage.hpp"

#include "storage/database_error.hpp"
#include "storage/in_memory/cursor.hpp"
#include "storage/in_memory/in_memory_batch.hpp"

using kagome::common::Buffer;

namespace kagome::storage {

  outcome::result<BufferOrView> InMemoryStorage::get(
      const BufferView &key) const {
    if (storage_.find(key) != storage_.end()) {
      return BufferView{storage_.at(key)};
    }

    return DatabaseError::NOT_FOUND;
  }

  outcome::result<std::optional<BufferOrView>> InMemoryStorage::tryGet(
      const common::BufferView &key) const {
    if (storage_.find(key) != storage_.end()) {
      return BufferView{storage_.at(key)};
    }

    return std::nullopt;
  }

  outcome::result<void> InMemoryStorage::put(const BufferView &key,
                                             BufferOrView &&value) {
    auto it = storage_.find(key);
    if (it != storage_.end()) {
      size_t old_value_size = it->second.size();
      BOOST_ASSERT(size_ >= old_value_size);
      size_ -= old_value_size;
    }
    size_ += value.size();
    storage_[key] = std::move(value).intoBuffer();
    return outcome::success();
  }

  outcome::result<bool> InMemoryStorage::contains(const BufferView &key) const {
    return storage_.find(key) != storage_.end();
  }

  outcome::result<void> InMemoryStorage::remove(const BufferView &key) {
    auto it = storage_.find(key);
    if (it != storage_.end()) {
      size_ -= it->second.size();
      storage_.erase(it);
    }
    return outcome::success();
  }

  outcome::result<void> InMemoryStorage::removePrefix(
      const common::BufferView &prefix) {
    auto start = storage_.lower_bound(prefix);
    auto end = storage_.upper_bound(prefix);
    storage_.erase(start, end);
    return outcome::success();
  }

  outcome::result<void> InMemoryStorage::clear() {
    storage_.clear();
    size_ = 0;
    return outcome::success();
  }

  std::unique_ptr<BufferBatch> InMemoryStorage::batch() {
    return std::make_unique<InMemoryBatch>(*this);
  }

  std::unique_ptr<InMemoryStorage::Cursor> InMemoryStorage::cursor() {
    return std::make_unique<InMemoryCursor>(*this);
  }

  std::optional<size_t> InMemoryStorage::byteSizeHint() const {
    return size_;
  }
}  // namespace kagome::storage
