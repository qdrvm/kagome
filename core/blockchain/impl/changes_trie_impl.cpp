/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/changes_trie_impl.hpp"

#include "scale/scale.hpp"

namespace kagome::blockchain {

  ChangesTrieImpl::ChangesTrieImpl(
      common::Hash256 parent,
      ChangesTrieConfig config,
      std::unique_ptr<storage::trie::TrieDb> changes_storage) {}

  outcome::result<void> ChangesTrieImpl::insertExtrinsicsChange(
      const common::Buffer &key,
      const std::vector<primitives::ExtrinsicIndex> &changers) {
    common::Buffer keyIndex;
    keyIndex.put(parent_);
    keyIndex.put(key);
    OUTCOME_TRY(value, scale::encode(changers));
    OUTCOME_TRY(changes_storage_->put(key, common::Buffer {std::move(value)}));
    return outcome::success();
  }

  common::Buffer ChangesTrieImpl::getRootHash() const {
    return changes_storage_->getRootHash();
  }

}  // namespace kagome::blockchain
