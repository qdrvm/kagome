/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IN_MEMORY_BATCH_HPP
#define KAGOME_IN_MEMORY_BATCH_HPP

#include "common/buffer.hpp"
#include "storage/in_memory/in_memory_storage.hpp"

namespace kagome::storage {

  class InMemoryBatch
      : public kagome::storage::face::WriteBatch<common::Buffer,
                                                 common::Buffer> {
   public:
    explicit InMemoryBatch(InMemoryStorage &db) : db{db} {}

    outcome::result<void> put(const common::Buffer &key,
                              const common::Buffer &value) override {
      entries[key.toHex()] = value;
      return outcome::success();
    }

    outcome::result<void> put(const common::Buffer &key,
                              common::Buffer &&value) override {
      entries[key.toHex()] = std::move(value);
      return outcome::success();
    }

    outcome::result<void> remove(const common::Buffer &key) override {
      entries.erase(key.toHex());
      return outcome::success();
    }

    outcome::result<void> commit() override {
      for (auto &entry : entries) {
        OUTCOME_TRY(
            db.put(common::Buffer::fromHex(entry.first).value(), entry.second));
      }
      return outcome::success();
    }

    void clear() override {
      entries.clear();
    }

   private:
    std::map<std::string, common::Buffer> entries;
    InMemoryStorage &db;
  };
}  // namespace kagome::storage

#endif  // KAGOME_IN_MEMORY_BATCH_HPP
