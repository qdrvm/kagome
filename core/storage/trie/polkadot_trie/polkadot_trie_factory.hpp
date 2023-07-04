/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_FACTORY
#define KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_FACTORY

#include "storage/trie/polkadot_trie/polkadot_trie.hpp"

namespace kagome::storage::trie {

  class PolkadotTrieFactory {
   public:
    /**
     * Creates an empty trie
     * @param f functor that a trie uses to obtain a child of a branch. If
     * optional is none, the default one will be used
     */
    virtual std::shared_ptr<PolkadotTrie> createEmpty(
        PolkadotTrie::RetrieveFunctions retrieve_functions = {}) const = 0;

    /**
     * Creates a trie with the given root
     * @param f functor that a trie uses to obtain a child of a branch. If
     * optional is none, the default one will be used
     */
    virtual std::shared_ptr<PolkadotTrie> createFromRoot(
        PolkadotTrie::NodePtr root,
        PolkadotTrie::RetrieveFunctions retrieve_functions = {}) const = 0;

    virtual ~PolkadotTrieFactory() = default;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_FACTORY
