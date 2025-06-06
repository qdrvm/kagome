/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "storage/trie/trie_storage.hpp"

namespace kagome::storage::trie {

  class TrieStorageMock : public TrieStorage {
   public:
    MOCK_METHOD(outcome::result<std::unique_ptr<TrieBatch>>,
                getPersistentBatchAt,
                (const storage::trie::RootHash &root, TrieChangesTrackerOpt),
                (override));

    MOCK_METHOD(outcome::result<std::unique_ptr<TrieBatch>>,
                getEphemeralBatchAt,
                (const storage::trie::RootHash &root),
                (const, override));

    MOCK_METHOD(outcome::result<std::unique_ptr<TrieBatch>>,
                getProofReaderBatchAt,
                (const RootHash &root, const OnNodeLoaded &on_node_loaded),
                (const, override));
  };

}  // namespace kagome::storage::trie
