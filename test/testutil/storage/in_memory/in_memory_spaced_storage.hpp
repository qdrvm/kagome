/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include <iostream>

#include "common/buffer.hpp"
#include "in_memory_storage.hpp"
#include "outcome/outcome.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/spaced_storage.hpp"
#include "testutil/storage/in_memory/in_memory_storage.hpp"

namespace kagome::storage {

  /**
   * Simple storage that conforms PersistentMap interface
   * Mostly needed to have an in-memory trie in tests to avoid integration with
   * an actual persistent database
   */
  class InMemorySpacedStorage final : public SpacedStorage {
   public:
    std::shared_ptr<BufferBatchableStorage> getSpace(Space space) override {
      auto it = spaces.find(space);
      if (it != spaces.end()) {
        return it->second;
      }
      return spaces.emplace(space, std::make_shared<InMemoryStorage>())
          .first->second;
    }

    std::unique_ptr<BufferSpacedBatch> createBatch() override;

   private:
    std::map<Space, std::shared_ptr<InMemoryStorage>> spaces;
  };

}  // namespace kagome::storage
