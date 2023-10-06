/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_IN_MEMORY_CURSOR_HPP
#define KAGOME_STORAGE_IN_MEMORY_CURSOR_HPP

#include "common/buffer.hpp"
#include "storage/in_memory/in_memory_storage.hpp"

namespace kagome::storage {
  class InMemoryCursor : public BufferStorageCursor {
   public:
    explicit InMemoryCursor(InMemoryStorage &db) : db{db} {}

    outcome::result<bool> seekFirst() override {
      return seek(db.storage.begin());
    }

    outcome::result<bool> seek(const BufferView &key) override {
      return seek(db.storage.lower_bound(key.toHex()));
    }

    outcome::result<bool> seekLast() override {
      return seek(db.storage.empty() ? db.storage.end()
                                     : std::prev(db.storage.end()));
    }

    bool isValid() const override {
      return kv.has_value();
    }

    outcome::result<void> next() override {
      seek(db.storage.upper_bound(kv->first.toHex()));
      return outcome::success();
    }

    outcome::result<void> prev() override {
      auto it = db.storage.lower_bound(kv->first.toHex());
      seek(it == db.storage.begin() ? db.storage.end() : std::prev(it));
      return outcome::success();
    }

    std::optional<Buffer> key() const override {
      if (kv) {
        return kv->first;
      }
      return std::nullopt;
    }

    std::optional<BufferOrView> value() const override {
      if (kv) {
        return BufferView{kv->second};
      }
      return std::nullopt;
    }

   private:
    bool seek(decltype(InMemoryStorage::storage)::iterator it) {
      if (it == db.storage.end()) {
        kv.reset();
      } else {
        kv.emplace(Buffer::fromHex(it->first).value(), it->second);
      }
      return isValid();
    }

    InMemoryStorage &db;
    std::optional<std::pair<Buffer, Buffer>> kv;
  };
}  // namespace kagome::storage

#endif  // KAGOME_STORAGE_IN_MEMORY_CURSOR_HPP
