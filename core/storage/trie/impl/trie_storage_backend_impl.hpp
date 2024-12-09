/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "storage/spaced_storage.hpp"
#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::trie {

  class TrieStorageBackendImpl : public TrieStorageBackend {
   public:
    TrieStorageBackendImpl(std::shared_ptr<SpacedStorage> db)
        : db_{std::move(db)} {
      BOOST_ASSERT(db_ != nullptr);
    }

    BufferStorage &nodes() override {
      return *db_->getSpace(Space::kTrieNode);
    }

    BufferStorage &values() override {
      return *db_->getSpace(Space::kTrieValue);
    }

    std::unique_ptr<BufferSpacedBatch> batch() override {
      return db_->createBatch();
    }

   private:
    std::shared_ptr<SpacedStorage> db_;
  };

}  // namespace kagome::storage::trie
