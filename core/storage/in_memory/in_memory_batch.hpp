/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IN_MEMORY_BATCH_HPP
#define KAGOME_IN_MEMORY_BATCH_HPP

#include "common/buffer.hpp"
#include "storage/in_memory/in_memory_storage.hpp"

namespace kagome::storage {
  using kagome::common::Buffer;

  class InMemoryBatch
      : public kagome::storage::face::WriteBatch<BufferView, Buffer> {
   public:
    explicit InMemoryBatch(InMemoryStorage &db) : db{db} {}

    outcome::result<void> put(const BufferView &key,
                              BufferOrView &&value) override {
      entries[key.toHex()] = value.into();
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

#endif  // KAGOME_IN_MEMORY_BATCH_HPP
