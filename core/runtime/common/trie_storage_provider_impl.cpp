/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/trie_storage_provider_impl.hpp"

namespace kagome::runtime {

  TrieStorageProviderImpl::TrieStorageProviderImpl(
      std::shared_ptr<storage::trie::TrieStorage> trie_storage)
      : trie_storage_(std::move(trie_storage)) {
    BOOST_ASSERT(trie_storage_ != nullptr);
  }

  outcome::result<void> TrieStorageProviderImpl::setToEphemeral() {
    OUTCOME_TRY(batch, trie_storage_->getEphemeralBatch());
    current_batch_ = std::move(batch);
    return outcome::success();
  }

  outcome::result<void> TrieStorageProviderImpl::setToEphemeralAt(
      const common::Hash256 &state_root) {
    OUTCOME_TRY(batch, trie_storage_->getEphemeralBatchAt(state_root));
    current_batch_ = std::move(batch);
    return outcome::success();
  }

  outcome::result<void> TrieStorageProviderImpl::setToPersistent() {
    if (persistent_batch_ == nullptr) {
      OUTCOME_TRY(batch, trie_storage_->getPersistentBatch());
      persistent_batch_ = std::move(batch);
    }
    current_batch_ = persistent_batch_;
    return outcome::success();
  }

  outcome::result<void> TrieStorageProviderImpl::setToPersistentAt(
      const common::Hash256 &state_root) {
    OUTCOME_TRY(batch, trie_storage_->getPersistentBatchAt(state_root));
    persistent_batch_ = std::move(batch);
    current_batch_ = persistent_batch_;
    return outcome::success();
  }

  std::shared_ptr<TrieStorageProviderImpl::Batch>
  TrieStorageProviderImpl::getCurrentBatch() const {
    return current_batch_;
  }

  boost::optional<std::shared_ptr<TrieStorageProviderImpl::PersistentBatch>>
  TrieStorageProviderImpl::tryGetPersistentBatch() const {
    return isCurrentlyPersistent() ? boost::make_optional(persistent_batch_)
                                   : boost::none;
  }

  bool TrieStorageProviderImpl::isCurrentlyPersistent() const {
    return std::dynamic_pointer_cast<PersistentBatch>(current_batch_)
           != nullptr;
  }
}  // namespace kagome::runtime
