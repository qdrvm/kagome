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
      std::unique_ptr<PoolModerator> moderator,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
      Limits limits, common::Logger logger)
      : header_repo_{std::move(header_repo)},
        logger_{std::move(logger)},
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
        provided_tags_by_.insert({tag, t.hash});
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

  outcome::result<std::vector<kagome::primitives::Transaction>>
  TransactionPoolImpl::removeStale(const primitives::BlockId &at) {
    OUTCOME_TRY(number, header_repo_->getNumberById(at));

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

  bool TransactionPoolImpl::isReady(const WaitingTransaction &tx) const {
    return std::all_of(
        tx.requires.begin(), tx.requires.end(), [this](auto &&tag) {
          return provided_tags_by_.find(tag) != provided_tags_by_.end();
        });
  };

  void TransactionPoolImpl::updateReady() {
    for (auto it = waiting_queue_.begin(); it != waiting_queue_.end();) {
      if (isReady(*it)) {
        Transaction tx = *it;
        ReadyTransaction rtx{tx, {}};
        updateUnlockingTransactions(rtx);
        ready_queue_.insert({rtx.hash, rtx});
        it = waiting_queue_.erase(it);
      } else {
        it++;
      }
    }
  }

  void TransactionPoolImpl::updateUnlockingTransactions(
      const ReadyTransaction &rtx) {
    for (auto &tag : rtx.requires) {
      if (auto it = provided_tags_by_.find(tag);
          it != provided_tags_by_.end() && it->second.has_value()) {
        if (auto ready_it = ready_queue_.find(it->second.value());
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

  std::vector<Transaction> TransactionPoolImpl::pruneTag(
      const primitives::BlockId &at, const primitives::TransactionTag &tag,
      const std::vector<common::Hash256> &known_imported_hashes) {
    provided_tags_by_.insert({tag, {}});
    updateReady();

    std::list<primitives::TransactionTag> to_remove{tag};
    std::vector<Transaction> removed;

    while (not to_remove.empty()) {
      auto tag_to_remove = to_remove.front();
      to_remove.pop_front();
      auto hash_opt = provided_tags_by_.at(tag_to_remove);
      provided_tags_by_.erase(tag_to_remove);
      if (not hash_opt) {
        continue;
      }
      auto tx = ready_queue_.at(hash_opt.value());
      ready_queue_.erase(hash_opt.value());

      auto find_previous = [this, &tx](primitives::TransactionTag const &tag)
          -> boost::optional<std::vector<primitives::TransactionTag>> {
        if (provided_tags_by_.find(tag) == provided_tags_by_.end()) {
          return boost::none;
        }
        auto prev_hash_opt = provided_tags_by_.at(tag);
        if (!prev_hash_opt) {
          return boost::none;
        }
        auto &tx2 = ready_queue_[prev_hash_opt.value()];
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
          return boost::make_optional(tx2.provides);
        }
        return boost::none;
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
    // make sure that we don't revalidate extrinsics that were part of the
    // recently imported block. This is especially important for UTXO-like
    // chains cause the inputs are pruned so such transaction would go to future
    // again.
    for (auto &hash : known_imported_hashes) {
      moderator_->ban(hash);
    }

    if(auto stale = removeStale(at); stale) {
      removed.insert(removed.end(), std::move_iterator(stale.value().begin()),
                     std::move_iterator(stale.value().end()));
    }
    return removed;
  }

}  // namespace kagome::transaction_pool
