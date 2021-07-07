/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/trie_storage_provider_impl.hpp"

#include "runtime/common/runtime_transaction_error.hpp"
#include "storage/trie/impl/topper_trie_batch_impl.hpp"

namespace kagome::runtime {
  using storage::trie::TopperTrieBatch;
  using storage::trie::TopperTrieBatchImpl;
  using storage::trie::TrieStorage;

  TrieStorageProviderImpl::TrieStorageProviderImpl(
      std::shared_ptr<TrieStorage> trie_storage)
      : trie_storage_{std::move(trie_storage)},
        logger_{log::createLogger("TrieStorageProvider", "runtime_api")} {
    BOOST_ASSERT(trie_storage_ != nullptr);
  }

  outcome::result<void> TrieStorageProviderImpl::setToEphemeral() {
    SL_DEBUG(logger_,
             "Setting storage provider to ephemeral batch with root {}",
             trie_storage_->getRootHash().toHex());
    OUTCOME_TRY(batch, trie_storage_->getEphemeralBatch());
    current_batch_ = std::move(batch);
    return outcome::success();
  }

  outcome::result<void> TrieStorageProviderImpl::setToEphemeralAt(
      const common::Hash256 &state_root) {
    SL_DEBUG(logger_,
             "Setting storage provider to ephemeral batch with root {}",
             state_root.toHex());
    OUTCOME_TRY(batch, trie_storage_->getEphemeralBatchAt(state_root));
    current_batch_ = std::move(batch);
    return outcome::success();
  }

  outcome::result<void> TrieStorageProviderImpl::setToPersistent() {
    if (persistent_batch_ == nullptr) {
      SL_DEBUG(logger_,
               "Setting storage provider to new persistent batch with root {}",
               trie_storage_->getRootHash().toHex());
      OUTCOME_TRY(batch, trie_storage_->getPersistentBatch());
      persistent_batch_ = std::move(batch);
    }
    SL_DEBUG(logger_,
             "Setting storage provider to existing persistent batch with root {}",
             trie_storage_->getRootHash().toHex());
    current_batch_ = persistent_batch_;
    return outcome::success();
  }

  outcome::result<void> TrieStorageProviderImpl::setToPersistentAt(
      const common::Hash256 &state_root) {
    SL_DEBUG(logger_,
             "Setting storage provider to new persistent batch with root {}",
             state_root.toHex());
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

  outcome::result<storage::trie::RootHash>
  TrieStorageProviderImpl::forceCommit() {
    if (persistent_batch_ != nullptr) {
      return persistent_batch_->commit();
    }
    return trie_storage_->getRootHash();
  }

  outcome::result<void> TrieStorageProviderImpl::startTransaction() {
    stack_of_batches_.emplace(current_batch_);
    current_batch_ =
        std::make_shared<TopperTrieBatchImpl>(std::move(current_batch_));
    return outcome::success();
  }

  outcome::result<void> TrieStorageProviderImpl::rollbackTransaction() {
    if (stack_of_batches_.empty()) {
      return RuntimeTransactionError::NO_TRANSACTIONS_WERE_STARTED;
    }

    current_batch_ = std::move(stack_of_batches_.top());
    stack_of_batches_.pop();
    return outcome::success();
  }

  outcome::result<void> TrieStorageProviderImpl::commitTransaction() {
    if (stack_of_batches_.empty()) {
      return RuntimeTransactionError::NO_TRANSACTIONS_WERE_STARTED;
    }

    auto commitee_batch =
        std::dynamic_pointer_cast<TopperTrieBatch>(current_batch_);
    BOOST_ASSERT(commitee_batch != nullptr);
    OUTCOME_TRY(commitee_batch->writeBack());

    current_batch_ = std::move(stack_of_batches_.top());
    stack_of_batches_.pop();
    return outcome::success();
  }

  storage::trie::RootHash TrieStorageProviderImpl::getLatestRoot()
      const noexcept {
    return trie_storage_->getRootHash();
  }

}  // namespace kagome::runtime
