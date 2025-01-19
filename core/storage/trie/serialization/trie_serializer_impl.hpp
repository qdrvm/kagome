/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/trie/serialization/trie_serializer.hpp"

#include "storage/buffer_map_types.hpp"

namespace kagome::storage::trie {
  class Codec;
  class PolkadotTrieFactory;
  class TrieStorageBackend;
  struct BranchNode;
  struct TrieNode;
}  // namespace kagome::storage::trie

namespace kagome::storage::trie {

  class TrieSerializerImpl : public TrieSerializer {
   public:
    TrieSerializerImpl(std::shared_ptr<PolkadotTrieFactory> factory,
                       std::shared_ptr<Codec> codec,
                       std::shared_ptr<TrieStorageBackend> node_backend);
    ~TrieSerializerImpl() override = default;

    RootHash getEmptyRootHash() const override;

    outcome::result<RootHash> storeTrie(PolkadotTrie &trie,
                                        StateVersion version) override;

    outcome::result<std::shared_ptr<PolkadotTrie>> retrieveTrie(
        RootHash db_key, OnNodeLoaded on_node_loaded) const override;

    /**
     * Fetches a node from the storage. A nullptr is returned in case that there
     * is no entry for provided key. Mind that a branch node will have dummy
     * nodes as its children
     */
    outcome::result<PolkadotTrie::NodePtr> retrieveNode(
        MerkleValue db_key, const OnNodeLoaded &on_node_loaded) const override;

    /**
     * Retrieves a node, replacing a dummy node to an actual node if
     * needed
     */
    outcome::result<PolkadotTrie::NodePtr> retrieveNode(
        const std::shared_ptr<OpaqueTrieNode> &node,
        const OnNodeLoaded &on_node_loaded) const override;

    outcome::result<std::optional<common::Buffer>> retrieveValue(
        const common::Hash256 &hash,
        const OnNodeLoaded &on_node_loaded) const override;

   private:
    /**
     * Writes a node to a persistent storage, recursively storing its
     * descendants as well. Then replaces the node children to dummy nodes to
     * avoid memory waste
     */
    outcome::result<RootHash> storeRootNode(TrieNode &node,
                                            StateVersion version);

    std::shared_ptr<PolkadotTrieFactory> trie_factory_;
    std::shared_ptr<Codec> codec_;
    std::shared_ptr<TrieStorageBackend> node_backend_;
  };
}  // namespace kagome::storage::trie
