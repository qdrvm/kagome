/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "storage/in_memory/in_memory_storage.hpp"

namespace kagome::storage {
  using kagome::common::Buffer;

  class InMemoryBatch : public BufferBatch {
   public:
    explicit InMemoryBatch(InMemoryStorage &db) : db{db} {}

    outcome::result<void> put(const BufferView &key,
                              BufferOrView &&value) override {
      entries[key.toHex()] = value.intoBuffer();
      return outcome::success();
    }

    outcome::result<void> remove(const BufferView &key) override {
      entries.erase(key.toHex());
      return outcome::success();
    }

    outcome::result<void> commit() override {
      for (auto &entry : entries) {
        OUTCOME_TRY(db.put(Buffer::fromHex(entry.first).value(),
                           BufferView{entry.second}));
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
