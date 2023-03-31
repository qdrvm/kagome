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
    setTo(std::move(batch));
    return outcome::success();
  }

  outcome::result<void> TrieStorageProviderImpl::setToPersistentAt(
      const common::Hash256 &state_root,
      TrieChangesTrackerOpt changes_tracker) {
    SL_DEBUG(logger_,
             "Setting storage provider to new persistent batch with root {}",
             state_root);
    OUTCOME_TRY(batch,
                trie_storage_->getPersistentBatchAt(
                    state_root, std::move(changes_tracker)));
    setTo(std::move(batch));
    return outcome::success();
  }

  void TrieStorageProviderImpl::setTo(
      std::shared_ptr<storage::trie::TrieBatch> batch) {
    SL_DEBUG(logger_, "Setting storage provider to new batch");
    child_batches_.clear();
    base_batch_ = batch;
    transaction_stack_.clear();
  }

  std::shared_ptr<TrieStorageProviderImpl::Batch>
  TrieStorageProviderImpl::getCurrentBatch() const {
    return transaction_stack_.empty() ? base_batch_
                                      : transaction_stack_.back().main_batch;
  }

  outcome::result<std::optional<std::shared_ptr<storage::trie::TrieBatch>>>
  TrieStorageProviderImpl::findChildBatchAt(
      const common::Buffer &root_path) const {
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
      return std::nullopt;
    }
    return child_it->second;
  }

  outcome::result<std::shared_ptr<storage::trie::TrieBatch>>
  TrieStorageProviderImpl::createBaseChildBatchAt(
      const common::Buffer &root_path) {
    SL_DEBUG(logger_,
             "Creating new base batch for child storage {}",
             root_path.toHex());
    if (child_batches_.count(root_path) != 0) {
      return child_batches_.at(root_path);
    }
    OUTCOME_TRY(child_batch, base_batch_->createChildBatch(root_path));
    // we've checked for duplicates above
    BOOST_ASSERT(child_batch.has_value());
    child_batches_.emplace(root_path, child_batch.value());
    return child_batch.value();
  }

  outcome::result<std::reference_wrapper<const storage::trie::TrieBatch>>
  TrieStorageProviderImpl::getChildBatchAt(const common::Buffer &root_path) {
    OUTCOME_TRY(batch_opt, findChildBatchAt(root_path));
    if (batch_opt.has_value()) {
      return **batch_opt;
    }
    OUTCOME_TRY(batch_ptr, createBaseChildBatchAt(root_path));
    return *batch_ptr;
  }

  outcome::result<std::reference_wrapper<storage::trie::TrieBatch>>
  TrieStorageProviderImpl::getMutableChildBatchAt(
      const common::Buffer &root_path) {
    // if we already have the batch, return it
    if (!transaction_stack_.empty()
        && transaction_stack_.back().child_batches.count(root_path) != 0) {
      return *transaction_stack_.back().child_batches.at(root_path);
    }

    // start with checking that the base batch exists, create one if it doesn't
    std::shared_ptr<storage::trie::TrieBatch> base_batch{};
    if (auto it = child_batches_.find(root_path); it == child_batches_.end()) {
      OUTCOME_TRY(batch_ptr, createBaseChildBatchAt(root_path));
      base_batch = batch_ptr;
    } else {
      base_batch = it->second;
    }
    auto highest_child_batch = base_batch;
    for (auto &transaction : transaction_stack_) {
      // if we have a batch at this level, just memorize it
      if (auto it = transaction.child_batches.find(root_path);
          it != transaction.child_batches.end()) {
        highest_child_batch = it->second;
      } else {
        // if we don't have a batch at this level, create one
        auto child_batch = std::make_shared<storage::trie::TopperTrieBatchImpl>(
            highest_child_batch);
        transaction.child_batches.insert({root_path, child_batch});
        highest_child_batch = child_batch;
      }
    }

    return *highest_child_batch;
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
