/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transaction_pool/impl/transaction_pool_impl.hpp"

#include "primitives/block_id.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

using kagome::primitives::BlockNumber;
using kagome::primitives::Transaction;

namespace kagome::transaction_pool {

  TransactionPoolImpl::TransactionPoolImpl(
      std::unique_ptr<PoolModerator> moderator, Limits limits,
      common::Logger logger)
      : logger_{std::move(logger)},
        moderator_{std::move(moderator)},
        limits_{limits} {}

  outcome::result<void> TransactionPoolImpl::submitOne(Transaction t) {
    return submit({t});
  }

  outcome::result<void> TransactionPoolImpl::submit(
      std::vector<Transaction> ts) {
    for (auto &t : ts) {
      auto hash = t.hash.toHex();
      if (imported_hashes_.find(hash) != imported_hashes_.end()) {
        return TransactionPoolError::ALREADY_IMPORTED;
      }
      for (auto &tag : t.provides) {
        provided_tags_.insert({tag, t.hash});
      }
      imported_hashes_.insert(hash);
      waiting_queue_.push_back(WaitingTransaction{std::move(t)});
    }

    updateReady();

    enforceLimits();

    return outcome::success();
  }

  std::vector<Transaction> TransactionPoolImpl::getReadyTransactions() {
    std::vector<Transaction> ready(ready_queue_.size());
    std::transform(ready_queue_.begin(), ready_queue_.end(), ready.begin(),
                   [](auto &rtx) { return rtx.second; });
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
        std::stable_partition(waiting_queue_.begin(), waiting_queue_.end(),
                              [this, number](auto &&tx) {
                                return moderator_->banIfStale(number, tx);
                              });
    std::move(waiting_queue_.begin(), w_border, std::back_inserter(removed));
    waiting_queue_.erase(waiting_queue_.begin(), w_border);

    for (auto &[hash, ready_tx] : ready_queue_) {
      if (moderator_->banIfStale(number, ready_tx)) {
        removed.push_back(ready_tx);
        ready_queue_.erase(hash);
      }
    }

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
    for (auto it = waiting_queue_.begin(); it != waiting_queue_.end();) {
      if (is_ready(*it)) {
        Transaction tx = *it;
        ReadyTransaction rtx{tx, {}};
        updateUnlockingTransactions(rtx);
        ready_queue_.insert({rtx.hash, rtx});
        auto old_it = it;
        it++;
        waiting_queue_.erase(old_it);
      } else {
        it++;
      }
    }
  }

  void TransactionPoolImpl::updateUnlockingTransactions(
      const ReadyTransaction &rtx) {
    for (auto &tag : rtx.requires) {
      if (auto it = provided_tags_.find(tag); it != provided_tags_.end()) {
        if (auto ready_it = ready_queue_.find(it->second);
            ready_it != ready_queue_.end()) {
          auto &unlocking_transaction = ready_it->second;
          unlocking_transaction.unlocks.push_back(rtx.hash);
        }
      }
    }
  }

  std::vector<primitives::Transaction> TransactionPoolImpl::enforceLimits() {
    auto [ready_num, waiting_num] = getStatus();
    int64_t dr = ready_num - limits_.max_ready_num;
    int64_t dw = waiting_num - limits_.max_waiting_num;
    std::vector<Transaction> removed;
    removed.reserve((dr > 0 ? dr : 0) + (dw > 0 ? dw : 0));
    while (ready_num > limits_.max_ready_num) {
      removed.push_back(ready_queue_.begin()->second);
      ready_queue_.erase(ready_queue_.begin());
      ready_num--;
    }
    while (waiting_num > limits_.max_waiting_num) {
      removed.push_back(waiting_queue_.front());
      waiting_queue_.pop_front();
      waiting_num--;
    }
    return removed;
  }

  TransactionPoolImpl::Status TransactionPoolImpl::getStatus() const {
    return Status{ready_queue_.size(), waiting_queue_.size()};
  }

  std::vector<Transaction> TransactionPoolImpl::pruneTags(
      const primitives::BlockId &at, const primitives::TransactionTag &tag,
      const std::vector<common::Hash256> &known_imported_hashes) {
    std::list<primitives::TransactionTag> to_remove{tag};
    std::vector<Transaction> removed;

    while (not to_remove.empty()) {
      auto tag = to_remove.front();
      to_remove.pop_front();
      auto hash = provided_tags_.at(tag);
      provided_tags_.erase(tag);
      auto tx = ready_queue_.at(hash);
      ready_queue_.erase(hash);

      auto find_previous = [this, &tx](primitives::TransactionTag const &tag)
          -> std::optional<std::vector<primitives::TransactionTag>> {
        auto prev_hash = provided_tags_.at(tag);
        auto &tx2 = ready_queue_[prev_hash];
        tx2.unlocks.remove_if([&tx](auto &t) { return t == tx.hash; });
        // We eagerly prune previous transactions as well.
        // But it might not always be good.
        // Possible edge case:
        // - tx provides two tags
        // - the second tag enables some subgraph we don't know of yet
        // - we will prune the transaction
        // - when we learn about the subgraph it will go to future
        // - we will have to wait for re-propagation of that transaction
        // Alternatively the caller may attempt to re-import these transactions.
        if (tx2.unlocks.empty()) {
          return std::optional(tx2.provides);
        }
        return std::nullopt;
      };
      for (auto &required_tag : tx.requires) {
        if (auto tags_to_remove = find_previous(required_tag); tags_to_remove) {
          std::copy(tags_to_remove.value().begin(),
                    tags_to_remove.value().end(),
                    std::back_inserter(to_remove));
        }
      }
      removed.push_back(tx);
    }
    return removed;
  }

}  // namespace kagome::transaction_pool
