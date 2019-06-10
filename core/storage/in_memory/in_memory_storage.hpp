/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_IN_MEMORY_IN_MEMORY_STORAGE_HPP
#define KAGOME_STORAGE_IN_MEMORY_IN_MEMORY_STORAGE_HPP

#include <memory>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "storage/face/persistent_map.hpp"
#include "storage/in_memory/in_memory_batch.hpp"

namespace kagome::storage {

  /**
   * Simple storage that conforms PersistentMap interface
   * Mostly needed to have an in-memory trie
   */
  class InMemoryStorage : public face::PersistentMap<Buffer, Buffer> {
   public:
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
      return std::make_unique<InMemoryBatch>(*this);
    }

    std::unique_ptr<kagome::storage::face::MapCursor<Buffer, Buffer>> cursor()
        override {
      return nullptr;
    }

  private:
    std::map<std::string, Buffer> storage;
  };

}  // namespace kagome::storage

#endif  // KAGOME_STORAGE_IN_MEMORY_IN_MEMORY_STORAGE_HPP
