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
    case E::UNFINISHED_TRANSACTIONS_LEFT:
      return "Attempt to commit state with unfinished transactions";
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
    return setTo(std::move(batch));
  }

  outcome::result<void> TrieStorageProviderImpl::setToPersistentAt(
      const common::Hash256 &state_root) {
    SL_DEBUG(logger_,
             "Setting storage provider to new persistent batch with root {}",
             state_root);
    OUTCOME_TRY(batch, trie_storage_->getPersistentBatchAt(state_root));
    return setTo(std::move(batch));
  }

  outcome::result<void> TrieStorageProviderImpl::setTo(
      std::shared_ptr<storage::trie::TrieBatch> batch) {
    SL_DEBUG(logger_, "Setting storage provider to new batch");
    child_batches_.clear();
    base_batch_ = batch;
    transaction_stack_.clear();
    return outcome::success();
  }

  std::shared_ptr<TrieStorageProviderImpl::Batch>
  TrieStorageProviderImpl::getCurrentBatch() const {
    return transaction_stack_.empty() ? base_batch_
                                      : transaction_stack_.back().main_batch;
  }

  outcome::result<std::shared_ptr<storage::trie::TrieBatch>>
  TrieStorageProviderImpl::getOrCreateChildBatchAt(
      const common::Buffer &root_path) {
    if (!transaction_stack_.empty()) {
      if (auto it = transaction_stack_.back().child_batches.find(root_path);
          it != transaction_stack_.back().child_batches.end()) {
        return it->second;
      }
    }
    for (auto transaction_it = transaction_stack_.rbegin();
         transaction_it != transaction_stack_.rend();
         transaction_it++) {
      if (auto it = transaction_it->child_batches.find(root_path);
          it != transaction_it->child_batches.end()) {
        return it->second;
      }
    }
    auto child_it = child_batches_.find(root_path);
    if (child_it == child_batches_.end()) {
      SL_DEBUG(logger_,
               "Creating new base batch for child storage {}",
               root_path.toHex());
      OUTCOME_TRY(child_batch, base_batch_->createChildBatch(root_path));
      // we check for duplicate batches in the if above
      BOOST_ASSERT(child_batch.has_value());
      child_batches_.emplace(root_path, child_batch.value());
      return child_batch.value();
    }
    SL_DEBUG(logger_, "Fetching batch for child storage {}", root_path);
    return child_it->second;
  }

  outcome::result<std::reference_wrapper<const storage::trie::TrieBatch>>
  TrieStorageProviderImpl::getChildBatchAt(const common::Buffer &root_path) {
    OUTCOME_TRY(batch_ptr, getOrCreateChildBatchAt(root_path));
    return *batch_ptr;
  }

  outcome::result<std::reference_wrapper<storage::trie::TrieBatch>>
  TrieStorageProviderImpl::getMutableChildBatchAt(
      const common::Buffer &root_path) {
    // if the batch for this child trie can be found in the current transaction,
    // just return it
    if (!transaction_stack_.empty()) {
      if (auto it = transaction_stack_.back().child_batches.find(root_path);
          it != transaction_stack_.back().child_batches.end()) {
        return *it->second;
      }
      // if there are no open transactions, just a base batch is sufficient
    } else {
      OUTCOME_TRY(batch_ptr, getOrCreateChildBatchAt(root_path));
      return *batch_ptr;
    }
    // otherwise we need to create a new overlay batch in the current
    // transaction
    OUTCOME_TRY(batch_ptr, getOrCreateChildBatchAt(root_path));

    SL_DEBUG(logger_,
             "Creating new overlay batch for child storage {}",
             root_path.toHex());
    auto child_batch =
        std::make_shared<storage::trie::TopperTrieBatchImpl>(batch_ptr);
    transaction_stack_.back().child_batches.emplace(root_path, child_batch);
    return *child_batch;
  }

  outcome::result<storage::trie::RootHash> TrieStorageProviderImpl::commit(
      StateVersion version) {
    if (!transaction_stack_.empty()) {
      return Error::UNFINISHED_TRANSACTIONS_LEFT;
    }
    if (base_batch_ != nullptr) {
      OUTCOME_TRY(root, base_batch_->commit(version));
      return std::move(root);
    }
    return Error::NO_BATCH;
  }

  outcome::result<void> TrieStorageProviderImpl::startTransaction() {
    transaction_stack_.emplace_back(Transaction{
        std::make_shared<TopperTrieBatchImpl>(getCurrentBatch()), {}});
    SL_TRACE(logger_,
             "Start storage transaction, depth {}",
             transaction_stack_.size());
    return outcome::success();
  }

  outcome::result<void> TrieStorageProviderImpl::rollbackTransaction() {
    if (transaction_stack_.empty()) {
      return RuntimeTransactionError::NO_TRANSACTIONS_WERE_STARTED;
    }

    SL_TRACE(logger_,
             "Rollback storage transaction, depth {}",
             transaction_stack_.size());
    transaction_stack_.pop_back();
    return outcome::success();
  }

  outcome::result<void> TrieStorageProviderImpl::commitTransaction() {
    if (transaction_stack_.empty()) {
      return RuntimeTransactionError::NO_TRANSACTIONS_WERE_STARTED;
    }

    OUTCOME_TRY(transaction_stack_.back().main_batch->writeBack());
    for (auto &[root, child_batch] : transaction_stack_.back().child_batches) {
      OUTCOME_TRY(child_batch->writeBack());
    }

    SL_TRACE(logger_,
             "Commit storage transaction, depth {}",
             transaction_stack_.size());
    transaction_stack_.pop_back();
    return outcome::success();
  }

}  // namespace kagome::runtime
