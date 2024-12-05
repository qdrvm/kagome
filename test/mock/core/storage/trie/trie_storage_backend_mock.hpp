/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::trie {

  class TrieStorageBackendMock : public TrieStorageBackend {
   public:
    MOCK_METHOD(BufferStorage &, nodes, (), (override));
    MOCK_METHOD(BufferStorage &, values, (), (override));
    MOCK_METHOD(std::unique_ptr<BufferSpacedBatch>, batch, (), (override));
  };

}  // namespace kagome::storage::trie
