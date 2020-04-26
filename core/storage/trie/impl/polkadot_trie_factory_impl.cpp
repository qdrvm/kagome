/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/polkadot_trie_factory_impl.hpp"

namespace kagome::storage::trie {

  std::unique_ptr<PolkadotTrie> PolkadotTrieFactoryImpl::createEmpty() const {
    return std::make_unique<PolkadotTrieImpl>(child_retrieve_f_);
  }

  std::unique_ptr<PolkadotTrie> PolkadotTrieFactoryImpl::createFromRoot(
      PolkadotTrie::NodePtr root) const {
    return std::make_unique<PolkadotTrieImpl>(std::move(root),
                                              child_retrieve_f_);
  }

}  // namespace kagome::storage::trie
