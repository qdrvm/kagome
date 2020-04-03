/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/changes_trie_builder_impl.hpp"

#include "scale/scale.hpp"

namespace kagome::blockchain {

  ChangesTrieBuilderImpl::ChangesTrieBuilderImpl(
      common::Hash256 parent,
      ChangesTrieConfig config,
      std::shared_ptr<storage::trie::TrieDbFactory> changes_storage_factory)
      : parent_{std::move(parent)},
        config_{std::move(config)},
        changes_storage_factory_{std::move(changes_storage_factory)} {
    BOOST_ASSERT(changes_storage_factory_ != nullptr);
    changes_storage_ = changes_storage_factory_->makeTrieDb();
  }

  ChangesTrieBuilder &ChangesTrieBuilderImpl::startNewTrie(
      common::Hash256 parent, boost::optional<ChangesTrieConfig> config) {
    changes_storage_ = changes_storage_factory_->makeTrieDb();
    parent_ = parent;
    if (config.has_value()) {
      config_ = config.value();
    }
    return *this;
  }

  outcome::result<void> ChangesTrieBuilderImpl::insertExtrinsicsChange(
      const common::Buffer &key,
      const std::vector<primitives::ExtrinsicIndex> &changers) {
    common::Buffer keyIndex;
    keyIndex.put(parent_);
    keyIndex.put(key);
    OUTCOME_TRY(value, scale::encode(changers));
    OUTCOME_TRY(changes_storage_->put(key, common::Buffer{std::move(value)}));
    return outcome::success();
  }

  common::Hash256 ChangesTrieBuilderImpl::finishAndGetHash() {
    auto hash_bytes = changes_storage_->getRootHash();
    changes_storage_.reset();
    common::Hash256 hash;
    std::copy_n(hash_bytes.begin(), common::Hash256::size(), hash.begin());
    return hash;
  }

}  // namespace kagome::blockchain
