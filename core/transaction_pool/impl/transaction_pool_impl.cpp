/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transaction_pool/impl/transaction_pool_impl.hpp"

#include "primitives/block_id.hpp"

using kagome::primitives::BlockNumber;
using kagome::primitives::Transaction;

OUTCOME_CPP_DEFINE_CATEGORY(kagome::transaction_pool,
                            TransactionPoolImpl::Error, e) {
  using E = kagome::transaction_pool::TransactionPoolImpl::Error;
  switch (e) {
    case E::ALREADY_IMPORTED:
      return "Transaction has already been imported to the pool";
  }
  return "Unknown error";
}

namespace kagome::transaction_pool {

  TransactionPoolImpl::TransactionPoolImpl(
      std::unique_ptr<PoolModerator> moderator,
      std::unique_ptr<Container<ReadyTransaction>> ready,
      std::unique_ptr<Container<WaitingTransaction>> waiting, Limits limits,
      common::Logger logger)
      : logger_{std::move(logger)},
        moderator_{std::move(moderator)},
        ready_queue_{std::move(ready)},
        waiting_queue_{std::move(waiting)},
        limits_{limits} {}

  outcome::result<void> TransactionPoolImpl::submitOne(Transaction t) {
    return submit({t});
  }

  outcome::result<void> TransactionPoolImpl::submit(
      std::vector<Transaction> ts) {
    for (auto &t : ts) {
      auto hash = t.hash.toHex();
      if (imported_hashes_.find(hash) != imported_hashes_.end()) {
        return Error::ALREADY_IMPORTED;
      }
      provided_tags_.insert(t.provides.begin(), t.provides.end());
      imported_hashes_.insert(hash);
      waiting_queue_->push_back(WaitingTransaction{std::move(t), {}});
    }

    updateReady();

    enforceLimits();

    return outcome::success();
  }

  std::vector<Transaction> TransactionPoolImpl::getReadyTransactions() {
    std::vector<Transaction> ready(ready_queue_->size());
    std::copy(ready_queue_->begin(), ready_queue_->end(), ready.begin());
    return ready;
  }

  std::vector<Transaction> TransactionPoolImpl::removeStale(
      const primitives::BlockId &at) {
    BlockNumber number;
    if (at.type() == typeid(BlockNumber)) {
      number = boost::get<BlockNumber>(at);
    } else {
      number = 0;
      // TODO(Harrm) Figure out how to obtain block number from block hash
    }

    std::vector<Transaction> removed;

    auto w_border =
        std::stable_partition(waiting_queue_->begin(), waiting_queue_->end(),
                              [this, number](auto &&tx) {
                                return moderator_->banIfStale(number, tx);
                              });
    std::move(waiting_queue_->begin(), w_border, std::back_inserter(removed));
    waiting_queue_->erase(waiting_queue_->begin(), w_border);

    auto r_border = std::stable_partition(
        ready_queue_->begin(), ready_queue_->end(), [this, number](auto &&tx) {
          return moderator_->banIfStale(number, tx);
        });
    std::move(ready_queue_->begin(), r_border, std::back_inserter(removed));
    ready_queue_->erase(ready_queue_->begin(), r_border);

    moderator_->updateBan();

    return removed;
  }

  void TransactionPoolImpl::updateReady() {
    auto is_ready = [this](auto &&tx) {
      return std::all_of(
          tx.requires.begin(), tx.requires.end(), [this](auto &&tag) {
            return provided_tags_.find(tag) != provided_tags_.end();
          });
    };
    auto border = std::stable_partition(waiting_queue_->begin(),
                                        waiting_queue_->end(), is_ready);
    std::transform(
        std::move_iterator(waiting_queue_->begin()), std::move_iterator(border),
        std::back_inserter(*ready_queue_), [](WaitingTransaction &&tx) {
          return ReadyTransaction{std::move(tx)};
        });
    waiting_queue_->erase(waiting_queue_->begin(), border);
  }

  std::vector<primitives::Transaction> TransactionPoolImpl::enforceLimits() {
    auto [ready_num, waiting_num] = getStatus();
    int64_t dr = ready_num - limits_.max_ready_num;
    int64_t dw = waiting_num - limits_.max_waiting_num;
    std::vector<Transaction> removed;
    removed.reserve((dr > 0 ? dr : 0) + (dw > 0 ? dw : 0));
    while (ready_num > limits_.max_ready_num) {
      removed.push_back(ready_queue_->pop_front());
      ready_num--;
    }
    while (waiting_num > limits_.max_waiting_num) {
      removed.push_back(waiting_queue_->pop_front());
      waiting_num--;
    }
    return removed;
  }

  TransactionPoolImpl::Status TransactionPoolImpl::getStatus() const {
    return Status{ready_queue_->size(), waiting_queue_->size()};
  }

  std::vector<Transaction> TransactionPoolImpl::prune(
      const primitives::BlockId &at,
      const std::vector<primitives::Extrinsic> &exts) {
    return {};
  }

  std::vector<Transaction> TransactionPoolImpl::pruneTags(
      const primitives::BlockId &at, const primitives::TransactionTag &tag,
      const std::vector<common::Hash256> &known_imported_hashes) {
    provided_tags_.insert(tag);
    updateReady();
    return {};
  }

}  // namespace kagome::transaction_pool
