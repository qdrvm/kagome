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
    using ChildRetrieveFunctor =
        std::function<outcome::result<PolkadotTrie::NodePtr>(
            PolkadotTrie::BranchPtr, uint8_t)>;

   protected:
    static outcome::result<PolkadotTrie::NodePtr> defaultChildRetriever(
        const PolkadotTrie::BranchPtr& branch, uint8_t idx) {
      return branch->children.at(idx);
    };

   public:
    /**
     * Creates an empty trie
     * @param f functor that a trie uses to obtain a child of a branch. If
     * optional is none, the default one will be used
     */
    virtual std::unique_ptr<PolkadotTrie> createEmpty(
        ChildRetrieveFunctor f = defaultChildRetriever) const = 0;

    /**
     * Creates a trie with the given root
     * @param f functor that a trie uses to obtain a child of a branch. If
     * optional is none, the default one will be used
     */
    virtual std::unique_ptr<PolkadotTrie> createFromRoot(
        PolkadotTrie::NodePtr root,
        ChildRetrieveFunctor f = defaultChildRetriever) const = 0;

    virtual ~PolkadotTrieFactory() = default;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_FACTORY
