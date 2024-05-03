/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/trie_storage_provider_impl.hpp"

#include "common/span_adl.hpp"
#include "runtime/common/runtime_execution_error.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/impl/topper_trie_batch_impl.hpp"
#include "storage/trie/trie_batches.hpp"

namespace kagome::runtime {
  using storage::kChildStoragePrefix;
  using storage::trie::TopperTrieBatchImpl;
  using storage::trie::TrieSerializer;
  using storage::trie::TrieStorage;

  inline bool starts_with_child_storage_key(const BufferView &key) {
    auto n = std::min(key.size(), kChildStoragePrefix.size());
    return SpanAdl{key.first(n)} == std::span{kChildStoragePrefix}.first(n);
  }

  TrieStorageProviderImpl::TrieStorageProviderImpl(
      std::shared_ptr<TrieStorage> trie_storage,
      std::shared_ptr<TrieSerializer> trie_serializer)
      : trie_storage_{std::move(trie_storage)},
        trie_serializer_{std::move(trie_serializer)},
        logger_{log::createLogger("TrieStorageProvider", "runtime_api")} {}

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
    transaction_stack_.emplace_back(
        Transaction{std::make_shared<TopperTrieBatchImpl>(base_batch_), {}});
  }

  std::shared_ptr<TrieStorageProviderImpl::Batch>
  TrieStorageProviderImpl::getCurrentBatch() const {
    if (transaction_stack_.empty()) {
      throw std::runtime_error("TrieStorageProviderImpl::getCurrentBatch");
    }
    return transaction_stack_.back().main_batch;
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
    auto it = child_batches_.find(root_path);
    if (it != child_batches_.end()) {
      return it->second;
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
    if (not transaction_stack_.empty()) {
      auto &top = transaction_stack_.back();
      auto it = top.child_batches.find(root_path);
      if (it != top.child_batches.end()) {
        return *it->second;
      }
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
      const std::optional<BufferView> &child, StateVersion version) {
    // TODO(turuslan): #2067, clone batch or implement delta_trie_root
    auto child_apply =
        [&](BufferView child,
            storage::BufferStorage &map) -> outcome::result<void> {
      for (auto &transaction : transaction_stack_) {
        auto it = transaction.child_batches.find(child);
        if (it == transaction.child_batches.end()) {
          continue;
        }
        OUTCOME_TRY(it->second->apply(map));
      }
      return outcome::success();
    };
    if (child) {
      OUTCOME_TRY(getChildBatchAt(*child));
      auto child_batch = child_batches_.at(*child);
      OUTCOME_TRY(child_apply(*child, *child_batch));
      return child_batch->commit(version);
    }
    auto batch = base_batch_;
    for (auto &transaction : transaction_stack_) {
      OUTCOME_TRY(transaction.main_batch->apply(*batch));
    }
    for (auto &p : child_batches_) {
      OUTCOME_TRY(getChildBatchAt(*child));
      auto child_batch = child_batches_.at(*child);
      OUTCOME_TRY(child_apply(p.first, *child_batch));
    }
    return batch->commit(version);
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
    if (transaction_stack_.size() <= 1) {
      return RuntimeExecutionError::NO_TRANSACTIONS_WERE_STARTED;
    }

    SL_TRACE(logger_,
             "Rollback storage transaction, depth {}",
             transaction_stack_.size());
    transaction_stack_.pop_back();
    return outcome::success();
  }

  outcome::result<void> TrieStorageProviderImpl::commitTransaction() {
    if (transaction_stack_.size() <= 1) {
      return RuntimeExecutionError::NO_TRANSACTIONS_WERE_STARTED;
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

  KillStorageResult TrieStorageProviderImpl::clearPrefix(
      const std::optional<BufferView> &child,
      BufferView prefix,
      const ClearPrefixLimit &limit) {
    KillStorageResult result;
    if (not child and starts_with_child_storage_key(prefix)) {
      return result;
    }
    storage::trie::TrieBatch *overlay;
    if (child) {
      overlay = &getMutableChildBatchAt(*child).value().get();
    } else {
      overlay = getCurrentBatch().get();
    }
    // https://github.com/paritytech/polkadot-sdk/blob/c973fe86f8c668462186c95655a58fda04508e9a/substrate/primitives/state-machine/src/overlayed_changes/mod.rs#L396-L399
    overlay->clearPrefix(prefix).value();
    std::unique_ptr<storage::BufferStorageCursor> cursor;
    if (child) {
      cursor = child_batches_.at(*child)->cursor();
    } else {
      cursor = base_batch_->cursor();
    }
    if (not cursor->seek(prefix)) {
      return result;
    }
    while (cursor->isValid()) {
      auto key = cursor->key().value();
      if (not startsWith(key, prefix)) {
        break;
      }
      if (limit and result.loops == *limit) {
        result.more = true;
        break;
      }
      overlay->remove(key).value();
      ++result.loops;
      if (not cursor->next()) {
        break;
      }
    }
    return result;
  }
}  // namespace kagome::runtime
