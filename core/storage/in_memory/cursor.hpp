/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "storage/in_memory/in_memory_storage.hpp"

namespace kagome::storage {
  class InMemoryCursor : public BufferStorageCursor {
   public:
    explicit InMemoryCursor(InMemoryStorage &db) : db{db} {}

    outcome::result<bool> seekFirst() override {
      return seek(db.storage_.begin());
    }

    outcome::result<bool> seek(const BufferView &key) override {
      return seek(db.storage_.find(key));
    }

    outcome::result<bool> seekLowerBound(const BufferView &key) override {
      return seek(db.storage_.lower_bound(key));
    }

    outcome::result<bool> seekLast() override {
      return seek(db.storage_.empty() ? db.storage_.end()
                                      : std::prev(db.storage_.end()));
    }

    bool isValid() const override {
      return kv.has_value();
    }

    outcome::result<void> next() override {
      seek(db.storage_.upper_bound(kv->first));
      return outcome::success();
    }

    outcome::result<void> prev() override {
      auto it = db.storage_.lower_bound(kv->first);
      seek(it == db.storage_.begin() ? db.storage_.end() : std::prev(it));
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
    bool seek(decltype(InMemoryStorage::storage_)::iterator it) {
      if (it == db.storage_.end()) {
        kv.reset();
      } else {
        kv.emplace(it->first, it->second);
      }
      return isValid();
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    InMemoryStorage &db;
    std::optional<std::pair<Buffer, Buffer>> kv;
  };
}  // namespace kagome::storage
