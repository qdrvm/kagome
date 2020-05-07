/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/polkadot_trie_factory_impl.hpp"

namespace kagome::storage::trie {

  PolkadotTrieFactoryImpl::PolkadotTrieFactoryImpl(
      PolkadotTrieImpl::ChildRetrieveFunctor f)
      : default_child_retrieve_f_{std::move(f)} {}

  std::unique_ptr<PolkadotTrie> PolkadotTrieFactoryImpl::createEmpty(
      boost::optional<ChildRetrieveFunctor> f) const {
    return std::make_unique<PolkadotTrieImpl>(
        f.has_value() ? f : default_child_retrieve_f_);
  }

  std::unique_ptr<PolkadotTrie> PolkadotTrieFactoryImpl::createFromRoot(
      PolkadotTrie::NodePtr root,
      boost::optional<ChildRetrieveFunctor> f) const {
    return std::make_unique<PolkadotTrieImpl>(
        std::move(root), f.has_value() ? f : default_child_retrieve_f_);
  }

}  // namespace kagome::storage::trie
