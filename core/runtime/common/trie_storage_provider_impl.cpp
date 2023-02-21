/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/trie_storage_provider_impl.hpp"

#include "runtime/common/runtime_transaction_error.hpp"
#include "storage/trie/impl/topper_trie_batch_impl.hpp"
#include "storage/trie/trie_batches.hpp"

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
  using storage::trie::TrieSerializer;
  using storage::trie::TrieStorage;

  TrieStorageProviderImpl::TrieStorageProviderImpl(
      std::shared_ptr<TrieStorage> trie_storage,
      std::shared_ptr<TrieSerializer> trie_serializer)
      : trie_storage_{std::move(trie_storage)},
        trie_serializer_{std::move(trie_serializer)},
        logger_{log::createLogger("TrieStorageProvider", "runtime_api")} {
    BOOST_ASSERT(trie_storage_ != nullptr);
  }

  outcome::result<void> TrieStorageProviderImpl::setToEphemeralAt(
      const common::Hash256 &state_root) {
    SL_DEBUG(logger_,
             "Setting storage provider to ephemeral batch with root {}",
             state_root);
    OUTCOME_TRY(batch, trie_storage_->getEphemeralBatchAt(state_root));
    return setTo(std::shared_ptr(std::move(batch)));
  }

  outcome::result<void> TrieStorageProviderImpl::setToPersistentAt(
      const common::Hash256 &state_root) {
    SL_DEBUG(logger_,
             "Setting storage provider to new persistent batch with root {}",
             state_root);
    OUTCOME_TRY(batch, trie_storage_->getPersistentBatchAt(state_root));
    return setTo(std::shared_ptr(std::move(batch)));
  }

  outcome::result<void> TrieStorageProviderImpl::setTo(
      std::shared_ptr<storage::trie::TrieBatch> batch) {
    SL_DEBUG(logger_, "Setting storage provider to new batch");
    child_batches_.clear();
    persistent_batch_ =
        std::dynamic_pointer_cast<storage::trie::PersistentTrieBatch>(batch);
    current_batch_ = batch;
    return outcome::success();
  }

  std::shared_ptr<TrieStorageProviderImpl::Batch>
  TrieStorageProviderImpl::getCurrentBatch() const {
    return current_batch_;
  }

  outcome::result<std::shared_ptr<storage::trie::TrieBatch>>
  TrieStorageProviderImpl::getChildBatchAt(const common::Buffer &root_path) {
    if (!child_batches_.count(root_path)) {
      SL_DEBUG(logger_,
               "Creating new persistent batch for child storage {}",
               root_path.toHex());
      OUTCOME_TRY(child_batch, current_batch_->createChildBatch(root_path));
      child_batches_.emplace(root_path, child_batch);
    }
    SL_DEBUG(
        logger_, "Fetching persistent batch for child storage {}", root_path);
    return child_batches_.at(root_path);
  }

  outcome::result<storage::trie::RootHash> TrieStorageProviderImpl::commit(
      StateVersion version) {
    if (persistent_batch_) {
      return persistent_batch_->commit(version);
    }
    if (auto ephemeral =
            std::dynamic_pointer_cast<storage::trie::EphemeralTrieBatch>(
                current_batch_)) {
      // won't actually write any data to the storage but will calculate the
      // root hash for the state represented by the batch
      OUTCOME_TRY(root, ephemeral->hash(version));
      return std::move(root);
    }
    return Error::NO_BATCH;
  }

  outcome::result<void> TrieStorageProviderImpl::startTransaction() {
    stack_of_batches_.emplace(current_batch_);
    SL_TRACE(logger_,
             "Start storage transaction, depth {}",
             stack_of_batches_.size());
    current_batch_ =
        std::make_shared<TopperTrieBatchImpl>(std::move(current_batch_));
    return outcome::success();
  }

  outcome::result<void> TrieStorageProviderImpl::rollbackTransaction() {
    if (stack_of_batches_.empty()) {
      return RuntimeTransactionError::NO_TRANSACTIONS_WERE_STARTED;
    }

    current_batch_ = std::move(stack_of_batches_.top());
    SL_TRACE(logger_,
             "Rollback storage transaction, depth {}",
             stack_of_batches_.size());
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
    SL_TRACE(logger_,
             "Commit storage transaction, depth {}",
             stack_of_batches_.size());
    stack_of_batches_.pop();
    return outcome::success();
  }

}  // namespace kagome::runtime
