/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_SERIALIZER_IMPL
#define KAGOME_STORAGE_TRIE_SERIALIZER_IMPL

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
                       std::shared_ptr<TrieStorageBackend> backend);
    ~TrieSerializerImpl() override = default;

    RootHash getEmptyRootHash() const override;

    outcome::result<RootHash> storeTrie(PolkadotTrie &trie,
                                        StateVersion version) override;

    outcome::result<std::shared_ptr<PolkadotTrie>> retrieveTrie(
        const common::Buffer &db_key,
        OnNodeLoaded on_node_loaded) const override;

   private:
    /**
     * Writes a node to a persistent storage, recursively storing its
     * descendants as well. Then replaces the node children to dummy nodes to
     * avoid memory waste
     */
    outcome::result<RootHash> storeRootNode(TrieNode &node,
                                            StateVersion version);
    /**
     * Fetches a node from the storage. A nullptr is returned in case that there
     * is no entry for provided key. Mind that a branch node will have dummy
     * nodes as its children
     */
    outcome::result<PolkadotTrie::NodePtr> retrieveNode(
        const common::Buffer &db_key, const OnNodeLoaded &on_node_loaded) const;

    /**
     * Retrieves a node, replacing a dummy node to an actual node if
     * needed
     */
    outcome::result<PolkadotTrie::NodePtr> retrieveNode(
        const std::shared_ptr<OpaqueTrieNode> &node,
        const OnNodeLoaded &on_node_loaded) const;

    std::shared_ptr<PolkadotTrieFactory> trie_factory_;
    std::shared_ptr<Codec> codec_;
    std::shared_ptr<TrieStorageBackend> backend_;
  };
}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_SERIALIZER_IMPL
