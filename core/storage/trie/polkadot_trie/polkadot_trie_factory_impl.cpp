/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"

namespace kagome::storage::trie {

  std::shared_ptr<PolkadotTrie> PolkadotTrieFactoryImpl::createEmpty(
      PolkadotTrie::NodeRetrieveFunctor f) const {
    return PolkadotTrieImpl::createEmpty(f);
  }

  std::shared_ptr<PolkadotTrie> PolkadotTrieFactoryImpl::createFromRoot(
      PolkadotTrie::NodePtr root, PolkadotTrie::NodeRetrieveFunctor f) const {
    return PolkadotTrieImpl::create(std::move(root), std::move(f));
  }

}  // namespace kagome::storage::trie
