/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_IMPL_POLKADOT_TRIE_FACTORY_IMPL
#define KAGOME_CORE_STORAGE_TRIE_IMPL_POLKADOT_TRIE_FACTORY_IMPL

#include "storage/trie/impl/polkadot_trie_factory.hpp"
#include "storage/trie/impl/polkadot_trie_impl.hpp"

namespace kagome::storage::trie {

  class PolkadotTrieFactoryImpl: public PolkadotTrieFactory {
   public:
    explicit PolkadotTrieFactoryImpl(PolkadotTrieImpl::ChildRetrieveFunctor f);

    std::unique_ptr<PolkadotTrie> createEmpty() const override;
    std::unique_ptr<PolkadotTrie> createFromRoot(
        PolkadotTrie::NodePtr root) const override;

   private:
    PolkadotTrieImpl::ChildRetrieveFunctor child_retrieve_f_;
  };

}

#endif  // KAGOME_CORE_STORAGE_TRIE_IMPL_POLKADOT_TRIE_FACTORY_IMPL
