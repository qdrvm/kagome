/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_POLKADOT_TRIE_SERIALIZER
#define KAGOME_STORAGE_POLKADOT_TRIE_SERIALIZER

#include "outcome/outcome.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie.hpp"
#include "storage/trie/types.hpp"

namespace kagome::storage::trie {

  /**
   * Serializes PolkadotTrie and stores it in an external storage
   */
  class TrieSerializer {
   public:
    virtual ~TrieSerializer() = default;

    /**
     * @return root hash of an empty trie
     */
    virtual RootHash getEmptyRootHash() const = 0;

    /**
     * Writes a trie to a storage, recursively storing its
     * nodes.
     */
    virtual outcome::result<RootHash> storeTrie(PolkadotTrie &trie,
                                                StateVersion version) = 0;

    /**
     * Fetches a trie from the storage. A nullptr is returned in case that there
     * is no entry for provided key.
     */
    virtual outcome::result<std::shared_ptr<PolkadotTrie>> retrieveTrie(
        const common::Buffer &db_key) const = 0;

    /**
     * Fetches a node from the storage. A nullptr is returned in case that there
     * is no entry for provided key. Mind that a branch node will have dummy
     * nodes as its children
     */
    virtual outcome::result<PolkadotTrie::NodePtr> retrieveNode(
        const common::Buffer &db_key) const = 0;

    /**
     * Retrieves a node, replacing a dummy node to an actual node if
     * needed
     */
    virtual outcome::result<PolkadotTrie::NodePtr> retrieveNode(
        const std::shared_ptr<OpaqueTrieNode> &node) const = 0;

  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_POLKADOT_TRIE_SERIALIZER
