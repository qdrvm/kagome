/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transaction_pool_impl.hpp"

using kagome::primitives::Transaction;

namespace kagome::transaction_pool {

  TransactionPoolImpl::TransactionPoolImpl(std::unique_ptr<Container> ready,
                                           std::unique_ptr<Container> waiting)
      : ready_queue_{std::move(ready)}, waiting_queue_{std::move(waiting)} {}

  outcome::result<void> TransactionPoolImpl::submitOne(Transaction t) {
    return submit({t});
  }

  outcome::result<void> TransactionPoolImpl::submit(
      std::vector<Transaction> ts) {
    for (auto &t : ts) {
      provided_tags_.insert(t.provides.begin(), t.provides.end());
      waiting_queue_->push_back(std::move(t));
    }

    updateReady();

    return outcome::success();
  }

  std::vector<Transaction> TransactionPoolImpl::getReadyTransactions() {
    std::vector<Transaction> ready(ready_queue_->size());
    std::copy(ready_queue_->begin(), ready_queue_->end(), ready.begin());
    return ready;
  }

  std::vector<Transaction> TransactionPoolImpl::removeStale(
      const primitives::BlockId &at) {
    return {};
  }

  std::vector<Transaction> TransactionPoolImpl::pruneByTag(
      primitives::TransactionTag tag) {
    return {};
  }

  void TransactionPoolImpl::updateReady() {
    auto is_ready = [this](auto &&tx) {
      return std::all_of(
          tx.requires.begin(), tx.requires.end(), [this](auto &&tag) {
            return provided_tags_.find(tag) == provided_tags_.end();
          });
    };
    auto border = std::stable_partition(waiting_queue_->begin(),
                                        waiting_queue_->end(), is_ready);
    std::move(waiting_queue_->begin(), std::move(border),
              std::back_inserter(*ready_queue_));
  }

  TransactionPoolImpl::Status TransactionPoolImpl::getStatus() const {
    return Status().ready(ready_queue_->size()).waiting(waiting_queue_->size());
  }

}  // namespace kagome::transaction_pool
