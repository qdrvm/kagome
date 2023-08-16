/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"

namespace kagome::storage::trie {

  std::shared_ptr<PolkadotTrie> PolkadotTrieFactoryImpl::createEmpty(
      PolkadotTrie::RetrieveFunctions retrieve_functions) const {
    return PolkadotTrieImpl::createEmpty(std::move(retrieve_functions));
  }

  std::shared_ptr<PolkadotTrie> PolkadotTrieFactoryImpl::createFromRoot(
      PolkadotTrie::NodePtr root,
      PolkadotTrie::RetrieveFunctions retrieve_functions) const {
    return PolkadotTrieImpl::create(std::move(root),
                                    std::move(retrieve_functions));
  }

}  // namespace kagome::storage::trie
