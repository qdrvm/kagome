/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/trie_storage_provider_impl.hpp"

#include "runtime/common/runtime_transaction_error.hpp"
#include "storage/trie/impl/topper_trie_batch_impl.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime,
                            TrieStorageProviderImpl::Error,
                            e) {
  using E = kagome::runtime::TrieStorageProviderImpl::Error;
  switch (e) {
    case E::NO_BATCH:
      return "Batch was not created or already was destructed.";
  }
  return "Unknown error";
}

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

  outcome::result<void> TrieStorageProviderImpl::setToEphemeralAt(
      const common::Hash256 &state_root) {
    SL_DEBUG(logger_,
             "Setting storage provider to ephemeral batch with root {}",
             state_root.toHex());
    OUTCOME_TRY(batch, trie_storage_->getEphemeralBatchAt(state_root));
    current_batch_ = std::move(batch);
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

  std::optional<std::shared_ptr<TrieStorageProviderImpl::PersistentBatch>>
  TrieStorageProviderImpl::tryGetPersistentBatch() const {
    return isCurrentlyPersistent() ? std::make_optional(persistent_batch_)
                                   : std::nullopt;
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
    return Error::NO_BATCH;
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

}  // namespace kagome::runtime
