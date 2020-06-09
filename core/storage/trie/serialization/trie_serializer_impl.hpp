/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_SERIALIZER_IMPL
#define KAGOME_STORAGE_TRIE_SERIALIZER_IMPL

#include "storage/trie/serialization/trie_serializer.hpp"

#include "storage/buffer_map_types.hpp"
#include "storage/trie/codec.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory.hpp"
#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::trie {

  class TrieSerializerImpl : public TrieSerializer {
   public:
    TrieSerializerImpl(std::shared_ptr<PolkadotTrieFactory> factory,
                       std::shared_ptr<Codec> codec,
                       std::shared_ptr<TrieStorageBackend> backend);
    ~TrieSerializerImpl() override = default;

    common::Buffer getEmptyRootHash() const override;

    outcome::result<common::Buffer> storeTrie(PolkadotTrie &trie) override;

    outcome::result<std::unique_ptr<PolkadotTrie>> retrieveTrie(
        const common::Buffer &db_key) const override;

   private:
    /**
     * Writes a node to a persistent storage, recursively storing its
     * descendants as well. Then replaces the node children to dummy nodes to
     * avoid memory waste
     */
    outcome::result<common::Buffer> storeRootNode(PolkadotNode &node);
    outcome::result<common::Buffer> storeNode(PolkadotNode &node,
                                              BufferBatch &batch);
    outcome::result<void> storeChildren(BranchNode &branch, BufferBatch &batch);
    /**
     * Fetches a node from the storage. A nullptr is returned in case that there
     * is no entry for provided key. Mind that a branch node will have dummy
     * nodes as its children
     */
    outcome::result<PolkadotTrie::NodePtr> retrieveNode(
        const common::Buffer &db_key) const;
    /**
     * Retrieves a node child, replacing a dummy node to an actual node if
     * needed
     */
    outcome::result<PolkadotTrie::NodePtr> retrieveChild(
        const PolkadotTrie::BranchPtr &parent, uint8_t idx) const;

    std::shared_ptr<PolkadotTrieFactory> trie_factory_;
    std::shared_ptr<Codec> codec_;
    std::shared_ptr<TrieStorageBackend> backend_;
  };
}  // namespace kagome::storage::trie

#endif // KAGOME_STORAGE_TRIE_SERIALIZER_IMPL
