/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
