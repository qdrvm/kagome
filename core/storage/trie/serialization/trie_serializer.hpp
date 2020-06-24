/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_POLKADOT_TRIE_SERIALIZER
#define KAGOME_STORAGE_POLKADOT_TRIE_SERIALIZER

#include "outcome/outcome.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie.hpp"

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
    virtual common::Buffer getEmptyRootHash() const = 0;

    /**
     * Writes a trie to a storage, recursively storing its
     * nodes.
     */
    virtual outcome::result<common::Buffer> storeTrie(PolkadotTrie &trie) = 0;

    /**
     * Fetches a trie from the storage. A nullptr is returned in case that there
     * is no entry for provided key.
     */
    virtual outcome::result<std::unique_ptr<PolkadotTrie>> retrieveTrie(
        const common::Buffer &db_key) const = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_POLKADOT_TRIE_SERIALIZER
