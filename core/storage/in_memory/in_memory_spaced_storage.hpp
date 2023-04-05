/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_INMEMORYSPACEDSTORAGE
#define KAGOME_STORAGE_INMEMORYSPACEDSTORAGE

#include <memory>

#include <iostream>

#include "common/buffer.hpp"
#include "in_memory_storage.hpp"
#include "outcome/outcome.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome::storage {

  /**
   * Simple storage that conforms PersistentMap interface
   * Mostly needed to have an in-memory trie in tests to avoid integration with
   * an actual persistent database
   */
  class InMemorySpacedStorage : public storage::SpacedStorage {
   public:
    ~InMemorySpacedStorage() override {
      std::cerr << "~InMemorySpacedStorage()" << std::endl;
    };

    std::shared_ptr<BufferStorage> getSpace(Space space) override {
      auto it = spaces.find(space);
      if (it != spaces.end()) {
        return it->second;
      }
      return spaces.emplace(space, std::make_shared<InMemoryStorage>())
          .first->second;
    }

   private:
    std::map<Space, std::shared_ptr<InMemoryStorage>> spaces;
  };

}  // namespace kagome::storage

#endif  // KAGOME_STORAGE_INMEMORYSPACEDSTORAGE
