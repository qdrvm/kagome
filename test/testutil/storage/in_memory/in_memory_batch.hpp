/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "storage/buffer_map_types.hpp"
#include "testutil/storage/in_memory/in_memory_spaced_storage.hpp"
#include "testutil/storage/in_memory/in_memory_storage.hpp"

namespace kagome::storage {
  using kagome::common::Buffer;

  class InMemoryBatch : public BufferBatch {
   public:
    explicit InMemoryBatch(InMemoryStorage &db) : db{db} {}

    outcome::result<void> put(const BufferView &key,
                              BufferOrView &&value) override {
      entries[key.toHex()] = std::move(value).intoBuffer();
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
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    InMemoryStorage &db;
  };

  class InMemorySpacedBatch : public BufferSpacedBatch {
   public:
    explicit InMemorySpacedBatch(SpacedStorage &db) : db{db} {}

    outcome::result<void> put(Space space,
                              const BufferView &key,
                              BufferOrView &&value) override {
      entries[std::make_pair(space, key.toHex())] =
          std::move(value).intoBuffer();
      return outcome::success();
    }

    outcome::result<void> remove(Space space, const BufferView &key) override {
      entries.erase(std::make_pair(space, key.toHex()));
      return outcome::success();
    }

    outcome::result<void> commit() override {
      for (auto &[key, entry] : entries) {
        OUTCOME_TRY(db.getSpace(key.first)->put(
            Buffer::fromHex(key.second).value(), BufferView{entry}));
      }
      return outcome::success();
    }

    void clear() override {
      entries.clear();
    }

   private:
    std::map<std::pair<Space, std::string>, Buffer> entries;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    SpacedStorage &db;
  };
}  // namespace kagome::storage
