/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_STORAGE_MERKLE_TRIE_DB_MAP_STORAGE_HPP_
#define KAGOME_TEST_CORE_STORAGE_MERKLE_TRIE_DB_MAP_STORAGE_HPP_

#include <memory>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "storage/face/persistent_map.hpp"

namespace test {
  using kagome::common::Buffer;

  /**
   * Simple storage that conforms PersistentMap interface to be used in Trie
   * tests instead of LevelDB
   */
  class MapDb : public kagome::storage::face::PersistentMap<Buffer, Buffer> {
   public:
    class Batch : public kagome::storage::face::WriteBatch<Buffer, Buffer> {
     public:
      explicit Batch(MapDb &db) : db{db} {}

      outcome::result<void> put(const Buffer &key,
                                const Buffer &value) override {
        entries[key.toHex()] = value;
        return outcome::success();
      }

      outcome::result<void> remove(const Buffer &key) override {
        entries.erase(key.toHex());
        return outcome::success();
      }

      outcome::result<void> commit() override {
        for (auto &entry : entries) {
          OUTCOME_TRY(
              db.put(Buffer::fromHex(entry.first).value(), entry.second));
        }
        return outcome::success();
      }

      void clear() override {
        entries.clear();
      }

     private:
      std::map<std::string, Buffer> entries;
      MapDb &db;
    };

    outcome::result<Buffer> get(const Buffer &key) const override {
      try {
        return storage.at(key.toHex());
      } catch (std::exception_ptr &e) {
        return outcome::error_from_exception(std::move(e));
      }
    }

    outcome::result<void> put(const Buffer &key, const Buffer &value) override {
      try {
        storage[key.toHex()] = value;
        return outcome::success();
      } catch (std::exception_ptr &e) {
        return outcome::error_from_exception(std::move(e));
      }
    }

    bool contains(const Buffer &key) const override {
      return storage.find(key.toHex()) != storage.end();
    }

    outcome::result<void> remove(const Buffer &key) override {
      storage.erase(key.toHex());
      return outcome::success();
    }

    std::unique_ptr<kagome::storage::face::WriteBatch<Buffer, Buffer>> batch()
        override {
      return std::make_unique<Batch>(*this);
    }

    std::unique_ptr<kagome::storage::face::MapCursor<Buffer, Buffer>> cursor()
        override {
      return nullptr;
    }

    std::map<std::string, Buffer> storage;
  };
}  // namespace test

#endif  // KAGOME_TEST_CORE_STORAGE_MERKLE_TRIE_DB_MAP_STORAGE_HPP_
