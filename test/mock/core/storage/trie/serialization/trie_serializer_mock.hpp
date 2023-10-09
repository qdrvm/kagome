/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "storage/trie/serialization/trie_serializer.hpp"

namespace kagome::storage::trie {

  class TrieSerializerMock : public TrieSerializer {
   public:
    MOCK_METHOD(RootHash, getEmptyRootHash, (), (const, override));

    MOCK_METHOD(outcome::result<RootHash>,
                storeTrie,
                (PolkadotTrie &, StateVersion),
                (override));

    MOCK_METHOD(outcome::result<std::shared_ptr<PolkadotTrie>>,
                retrieveTrie,
                (RootHash, OnNodeLoaded),
                (const, override));

    MOCK_METHOD(outcome::result<PolkadotTrie::NodePtr>,
                retrieveNode,
                (MerkleValue db_key, const OnNodeLoaded &on_node_loaded),
                (const, override));

    MOCK_METHOD(outcome::result<PolkadotTrie::NodePtr>,
                retrieveNode,
                (const std::shared_ptr<OpaqueTrieNode> &node,
                 const OnNodeLoaded &on_node_loaded),
                (const, override));

    MOCK_METHOD(outcome::result<std::optional<common::Buffer>>,
                retrieveValue,
                (const common::Hash256 &hash,
                 const OnNodeLoaded &on_node_loaded),
                (const));
  };

}  // namespace kagome::storage::trie
