/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transaction_pool/impl/transaction_pool_impl.hpp"

#include "crypto/hasher.hpp"
#include "network/transactions_transmitter.hpp"
#include "primitives/block_id.hpp"
#include "runtime/runtime_api/tagged_transaction_queue.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

using kagome::primitives::BlockNumber;
using kagome::primitives::Transaction;

namespace {
  constexpr const char *readyTransactionsMetricName =
      "kagome_ready_transactions_number";
}

namespace kagome::transaction_pool {

  using primitives::events::ExtrinsicEventType;
  using primitives::events::ExtrinsicLifecycleEvent;

  TransactionPoolImpl::TransactionPoolImpl(
      std::shared_ptr<runtime::TaggedTransactionQueue> ttq,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<network::TransactionsTransmitter> tx_transmitter,
      std::unique_ptr<PoolModerator> moderator,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
          sub_engine,
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository> ext_key_repo,
      Limits limits)
      : header_repo_{std::move(header_repo)},
        sub_engine_{std::move(sub_engine)},
        ext_key_repo_{std::move(ext_key_repo)},
        ttq_{std::move(ttq)},
        hasher_{std::move(hasher)},
        tx_transmitter_{std::move(tx_transmitter)},
        moderator_{std::move(moderator)},
        limits_{limits} {
    BOOST_ASSERT_MSG(header_repo_ != nullptr, "header repo is nullptr");
    BOOST_ASSERT_MSG(ttq_ != nullptr, "tagged-transaction queue is nullptr");
    BOOST_ASSERT_MSG(hasher_ != nullptr, "hasher is nullptr");
    BOOST_ASSERT_MSG(tx_transmitter_ != nullptr, "tx_transmitter is nullptr");
    BOOST_ASSERT_MSG(moderator_ != nullptr, "moderator is nullptr");
    BOOST_ASSERT_MSG(sub_engine_ != nullptr, "sub engine is nullptr");
    BOOST_ASSERT_MSG(ext_key_repo_ != nullptr,
                     "extrinsic event key repository is nullptr");

    // Register metrics
    metrics_registry_->registerGaugeFamily(
        readyTransactionsMetricName,
        "Number of transactions in the ready queue");
    metric_ready_txs_ =
        metrics_registry_->registerGaugeMetric(readyTransactionsMetricName);
    metric_ready_txs_->set(0);
  }

  outcome::result<primitives::Transaction>
  TransactionPoolImpl::constructTransaction(
      primitives::TransactionSource source,
      primitives::Extrinsic extrinsic) const {
    OUTCOME_TRY(res, ttq_->validate_transaction(source, extrinsic));

    return visit_in_place(
        res,
        [&](const primitives::TransactionValidityError &e) {
          return visit_in_place(
              e,
              // return either invalid or unknown validity error
              [](const auto &validity_error)
                  -> outcome::result<primitives::Transaction> {
                return validity_error;
              });
        },
        [&](const primitives::ValidTransaction &v)
            -> outcome::result<primitives::Transaction> {
          common::Hash256 hash = hasher_->blake2b_256(extrinsic.data);
          size_t length = extrinsic.data.size();

          return primitives::Transaction{extrinsic,
                                         length,
                                         hash,
                                         v.priority,
                                         v.longevity,
                                         v.requires,
                                         v.provides,
                                         v.propagate};
        });
  }

  outcome::result<Transaction::Hash> TransactionPoolImpl::submitExtrinsic(
      primitives::TransactionSource source, primitives::Extrinsic extrinsic) {
    OUTCOME_TRY(tx, constructTransaction(source, extrinsic));

    if (tx.should_propagate) {
      tx_transmitter_->propagateTransactions(gsl::make_span(std::vector{tx}));
    }
    auto hash = tx.hash;
    // send to pool
    OUTCOME_TRY(submitOne(std::move(tx)));

    return hash;
  }

  outcome::result<void> TransactionPoolImpl::submitOne(Transaction &&tx) {
    return submitOne(std::make_shared<Transaction>(std::move(tx)));
  }

  outcome::result<void> TransactionPoolImpl::submitOne(
      const std::shared_ptr<Transaction> &tx) {
    if (auto [_, ok] = imported_txs_.emplace(tx->hash, tx); !ok) {
      return TransactionPoolError::TX_ALREADY_IMPORTED;
    }

    auto processResult = processTransaction(tx);
    if (processResult.has_error()
        && processResult.error() == TransactionPoolError::POOL_IS_FULL) {
      if (auto key = ext_key_repo_->get(tx->hash); key.has_value()) {
        sub_engine_->notify(key.value(),
                            ExtrinsicLifecycleEvent::Dropped(key.value()));
      }
      imported_txs_.erase(tx->hash);
    } else {
      SL_DEBUG(logger_,
               "Extrinsic {} with hash {} was added to the pool",
               tx->ext.data.toHex(),
               tx->hash.toHex());
    }

    return processResult;
  }

  outcome::result<void> TransactionPoolImpl::processTransaction(
      const std::shared_ptr<Transaction> &tx) {
    OUTCOME_TRY(ensureSpace());
    if (checkForReady(tx)) {
      OUTCOME_TRY(processTransactionAsReady(tx));
    } else {
      OUTCOME_TRY(processTransactionAsWaiting(tx));
    }
    return outcome::success();
  }

  outcome::result<void> TransactionPoolImpl::processTransactionAsReady(
      const std::shared_ptr<Transaction> &tx) {
    if (hasSpaceInReady()) {
      setReady(tx);
    } else {
      postponeTransaction(tx);
    }

    return outcome::success();
  }

  bool TransactionPoolImpl::hasSpaceInReady() const {
    return ready_txs_.size() < limits_.max_ready_num;
  }

  void TransactionPoolImpl::postponeTransaction(
      const std::shared_ptr<Transaction> &tx) {
    postponed_txs_.push_back(tx);
    if (auto key = ext_key_repo_->get(tx->hash); key.has_value()) {
      sub_engine_->notify(key.value(),
                          ExtrinsicLifecycleEvent::Future(key.value()));
    }
  }

  outcome::result<void> TransactionPoolImpl::processTransactionAsWaiting(
      const std::shared_ptr<Transaction> &tx) {
    OUTCOME_TRY(ensureSpace());

    addTransactionAsWaiting(tx);

    return outcome::success();
  }

  outcome::result<void> TransactionPoolImpl::ensureSpace() const {
    if (imported_txs_.size() > limits_.capacity) {
      return TransactionPoolError::POOL_IS_FULL;
    }

    return outcome::success();
  }

  void TransactionPoolImpl::addTransactionAsWaiting(
      const std::shared_ptr<Transaction> &tx) {
    for (auto &tag : tx->requires) {
      tx_waits_tag_.emplace(tag, tx);
    }
    if (auto key = ext_key_repo_->get(tx->hash); key.has_value()) {
      sub_engine_->notify(key.value(),
                          ExtrinsicLifecycleEvent::Future(key.value()));
    }
  }

  outcome::result<Transaction> TransactionPoolImpl::removeOne(
      const Transaction::Hash &tx_hash) {
    auto tx_node = imported_txs_.extract(tx_hash);
    if (tx_node.empty()) {
      SL_TRACE(logger_,
               "Extrinsic with hash {} was not found in the pool during remove",
               tx_hash);
      return TransactionPoolError::TX_NOT_FOUND;
    }
    auto &tx = tx_node.mapped();

    unsetReady(tx);
    delTransactionAsWaiting(tx);

    processPostponedTransactions();

    SL_DEBUG(logger_,
             "Extrinsic {} with hash {} was removed from the pool",
             tx->ext.data.toHex(),
             tx->hash.toHex());
    return std::move(*tx);
  }

  void TransactionPoolImpl::processPostponedTransactions() {
    // Move to local for avoid endless cycle at possible coming back tx
    auto postponed_txs = std::move(postponed_txs_);
    while (!postponed_txs.empty()) {
      auto tx = postponed_txs.front().lock();
      postponed_txs.pop_front();

      auto result = processTransaction(tx);
      if (result.has_error()
          && result.error() == TransactionPoolError::POOL_IS_FULL) {
        postponed_txs_.insert(postponed_txs_.end(),
                              std::make_move_iterator(postponed_txs.begin()),
                              std::make_move_iterator(postponed_txs.end()));
        return;
      }
    }
  }

  void TransactionPoolImpl::delTransactionAsWaiting(
      const std::shared_ptr<Transaction> &tx) {
    for (auto &tag : tx->requires) {
      auto range = tx_waits_tag_.equal_range(tag);
      for (auto i = range.first; i != range.second;) {
        if (i->second.lock() == tx) {
          tx_waits_tag_.erase(i);
          break;
        }
      }
    }
  }

  std::map<Transaction::Hash, std::shared_ptr<Transaction>>
  TransactionPoolImpl::getReadyTransactions() const {
    std::map<Transaction::Hash, std::shared_ptr<Transaction>> ready;
    std::for_each(ready_txs_.begin(), ready_txs_.end(), [&ready](auto it) {
      if (auto tx = it.second.lock()) {
        ready.emplace(it.first, std::move(tx));
      }
    });
    return ready;
  }

  const std::unordered_map<Transaction::Hash, std::shared_ptr<Transaction>>
      &TransactionPoolImpl::getPendingTransactions() const {
    return imported_txs_;
  }

  outcome::result<std::vector<Transaction>> TransactionPoolImpl::removeStale(
      const primitives::BlockId &at) {
    OUTCOME_TRY(number, header_repo_->getNumberById(at));

    std::vector<Transaction::Hash> remove_to;

    for (auto &[txHash, tx] : imported_txs_) {
      if (moderator_->banIfStale(number, *tx)) {
        remove_to.emplace_back(txHash);
      }
    }

    for (auto &tx_hash : remove_to) {
      OUTCOME_TRY(tx, removeOne(tx_hash));
      if (auto key = ext_key_repo_->get(tx.hash); key.has_value()) {
        sub_engine_->notify(key.value(),
                            ExtrinsicLifecycleEvent::Dropped(key.value()));
        ext_key_repo_->remove(tx.hash);
      }
    }

    moderator_->updateBan();

    return outcome::success();
  }

  bool TransactionPoolImpl::isInReady(
      const std::shared_ptr<const Transaction> &tx) const {
    auto i = ready_txs_.find(tx->hash);
    return i != ready_txs_.end() && !i->second.expired();
  }

  bool TransactionPoolImpl::checkForReady(
      const std::shared_ptr<const Transaction> &tx) const {
    return std::all_of(
        tx->requires.begin(), tx->requires.end(), [this](auto &&tag) {
          auto range = tx_provides_tag_.equal_range(tag);
          return range.first != range.second;
        });
  }

  void TransactionPoolImpl::setReady(const std::shared_ptr<Transaction> &tx) {
    if (auto [_, ok] = ready_txs_.emplace(tx->hash, tx); ok) {
      if (auto key = ext_key_repo_->get(tx->hash); key.has_value()) {
        sub_engine_->notify(key.value(),
                            ExtrinsicLifecycleEvent::Ready(key.value()));
      }
      commitRequiredTags(tx);
      commitProvidedTags(tx);
      metric_ready_txs_->set(ready_txs_.size());
    }
  }

  void TransactionPoolImpl::commitRequiredTags(
      const std::shared_ptr<Transaction> &tx) {
    for (auto &tag : tx->requires) {
      auto range = tx_waits_tag_.equal_range(tag);
      for (auto i = range.first; i != range.second;) {
        auto ci = i++;
        if (ci->second.lock() == tx) {
          auto node = tx_waits_tag_.extract(ci);
          tx_depends_on_tag_.emplace(std::move(node.key()),
                                     std::move(node.mapped()));
        }
      }
    }
  }

  void TransactionPoolImpl::commitProvidedTags(
      const std::shared_ptr<Transaction> &tx) {
    for (auto &tag : tx->provides) {
      tx_provides_tag_.emplace(tag, tx);

      provideTag(tag);
    }
  }

  void TransactionPoolImpl::provideTag(const Transaction::Tag &tag) {
    auto range = tx_waits_tag_.equal_range(tag);
    for (auto it = range.first; it != range.second;) {
      auto tx = (it++)->second.lock();
      if (checkForReady(tx)) {
        if (hasSpaceInReady()) {
          setReady(tx);
        } else {
          postponeTransaction(tx);
        }
      }
    }
  }

  void TransactionPoolImpl::unsetReady(const std::shared_ptr<Transaction> &tx) {
    if (auto tx_node = ready_txs_.extract(tx->hash); !tx_node.empty()) {
      metric_ready_txs_->set(ready_txs_.size());
      rollbackRequiredTags(tx);
      rollbackProvidedTags(tx);
      if (auto key = ext_key_repo_->get(tx->hash); key.has_value()) {
        sub_engine_->notify(key.value(),
                            ExtrinsicLifecycleEvent::Future(key.value()));
      }
    }
  }

  void TransactionPoolImpl::rollbackRequiredTags(
      const std::shared_ptr<Transaction> &tx) {
    for (auto &tag : tx->requires) {
      tx_waits_tag_.emplace(tag, tx);
    }
  }

  void TransactionPoolImpl::rollbackProvidedTags(
      const std::shared_ptr<Transaction> &tx) {
    for (auto &tag : tx->provides) {
      auto range = tx_provides_tag_.equal_range(tag);
      for (auto i = range.first; i != range.second; ++i) {
        if (i->second.lock() == tx) {
          tx_provides_tag_.erase(i);
          break;
        }
      }

      unprovideTag(tag);
    }
  }

  void TransactionPoolImpl::unprovideTag(const Transaction::Tag &tag) {
    if (tx_provides_tag_.find(tag) == tx_provides_tag_.end()) {
      for (auto it = tx_depends_on_tag_.find(tag);
           it != tx_depends_on_tag_.end();
           it = tx_depends_on_tag_.find(tag)) {
        if (auto tx = it->second.lock()) {
          unsetReady(tx);
        }
        tx_depends_on_tag_.erase(it);
      }
    }
  }

  TransactionPoolImpl::Status TransactionPoolImpl::getStatus() const {
    return Status{ready_txs_.size(), imported_txs_.size() - ready_txs_.size()};
  }

}  // namespace kagome::transaction_pool
