/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/trie/polkadot_trie/polkadot_trie_factory.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"

namespace kagome::storage::trie {

  class PolkadotTrieFactoryImpl : public PolkadotTrieFactory {
   public:
    std::shared_ptr<PolkadotTrie> createEmpty(
        PolkadotTrie::RetrieveFunctions retrieve_functions) const override;

    std::shared_ptr<PolkadotTrie> createFromRoot(
        PolkadotTrie::NodePtr root,
        PolkadotTrie::RetrieveFunctions retrieve_functions) const override;
  };

}  // namespace kagome::storage::trie
