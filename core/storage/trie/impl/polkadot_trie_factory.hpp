/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_FACTORY
#define KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_FACTORY

#include "storage/trie/impl/polkadot_trie.hpp"

namespace kagome::storage::trie {

  class PolkadotTrieFactory {
   public:
    virtual std::unique_ptr<PolkadotTrie> createEmpty() const = 0;
    virtual std::unique_ptr<PolkadotTrie> createFromRoot(
        PolkadotTrie::NodePtr root) const = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_FACTORY
