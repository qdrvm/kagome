/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
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
    return constructTransaction(
        source, extrinsic, hasher_->blake2b_256(extrinsic.data));
  }

  outcome::result<Transaction> TransactionPoolImpl::constructTransaction(
      primitives::TransactionSource source,
      primitives::Extrinsic extrinsic,
      const Transaction::Hash &extrinsic_hash) const {
    OUTCOME_TRY(res, ttq_->validate_transaction(source, extrinsic));

    return visit_in_place(
        std::move(res.second),
        [&](const primitives::TransactionValidityError &e) {
          return visit_in_place(
              e,
              // return either invalid or unknown validity error
              [](const primitives::InvalidTransaction &validity_error)
                  -> outcome::result<primitives::Transaction> {
                return validity_error.kind;
              },
              [](const primitives::UnknownTransaction &validity_error)
                  -> outcome::result<primitives::Transaction> {
                return validity_error.kind;
              });
        },
        [&](primitives::ValidTransaction &&v)
            -> outcome::result<primitives::Transaction> {
          size_t length = extrinsic.data.size();

          return primitives::Transaction{
              .ext = extrinsic,
              .bytes = length,
              .hash = extrinsic_hash,
              .priority = v.priority,
              .valid_till = res.first.number + v.longevity,
              .required_tags = std::move(v.required_tags),
              .provided_tags = std::move(v.provided_tags),
              .should_propagate = v.propagate};
        });
  }

  bool TransactionPoolImpl::imported(const Transaction::Hash &tx_hash) const {
    return pool_state_.sharedAccess([&](const auto &pool_state) {
      return pool_state.pending_txs_.find(tx_hash)
              != pool_state.pending_txs_.end()
          || pool_state.ready_txs_.find(tx_hash) != pool_state.ready_txs_.end();
    });
  }

  outcome::result<Transaction::Hash> TransactionPoolImpl::submitExtrinsic(
      primitives::TransactionSource source, primitives::Extrinsic extrinsic) {
    auto hash = hasher_->blake2b_256(extrinsic.data);
    if (imported(hash)) {
      return TransactionPoolError::TX_ALREADY_IMPORTED;
    }
    OUTCOME_TRY(tx, constructTransaction(source, extrinsic, hash));

    if (tx.should_propagate) {
      tx_transmitter_->propagateTransaction(tx);
    }

    OUTCOME_TRY(
        submitOneInternal(std::make_shared<Transaction>(std::move(tx))));

    return hash;
  }

  outcome::result<void> TransactionPoolImpl::submitOne(Transaction &&tx) {
    if (imported(tx.hash)) {
      return TransactionPoolError::TX_ALREADY_IMPORTED;
    }
    return submitOneInternal(std::make_shared<Transaction>(std::move(tx)));
  }

  size_t TransactionPoolImpl::imported_txs_count() const {
    return pool_state_.sharedAccess([&](const auto &pool_state) {
      return pool_state.pending_txs_.size() + pool_state.ready_txs_.size();
    });
  }

  outcome::result<void> TransactionPoolImpl::submitOneInternal(
      const std::shared_ptr<Transaction> &tx) {
    if (imported_txs_count() >= limits_.capacity) {
      if (auto key = ext_key_repo_->get(tx->hash); key.has_value()) {
        sub_engine_->notify(key.value(),
                            ExtrinsicLifecycleEvent::Dropped(key.value()));
      }
      return TransactionPoolError::POOL_IS_FULL;
    }

    SL_DEBUG(logger_,
             "Extrinsic {} with hash {} was added to the pool",
             tx->ext.data.toHex(),
             tx->hash.toHex());

    return processTransaction(tx);
  }

  bool TransactionPoolImpl::is_ready(
      const PoolState &pool_state,
      const std::shared_ptr<const Transaction> &tx) const {
    return std::all_of(tx->required_tags.begin(),
                       tx->required_tags.end(),
                       [&](const auto &tag) {
                         auto it = pool_state.dependency_graph_.find(tag);
                         return it != pool_state.dependency_graph_.end()
                             && it->second.tag_provided;
                       });
  }

  outcome::result<void> TransactionPoolImpl::processTransaction(
      const std::shared_ptr<Transaction> &tx) {
    pool_state_.exclusiveAccess([&](auto &pool_state) {
      if (is_ready(pool_state, tx)) {
        setReady(pool_state, tx);
        return;
      }
      auto state = std::make_shared<TxReadyState>(tx);
      pool_state.pending_txs_[tx->hash] = state;
      for (auto &tag : tx->required_tags) {
        auto &pending_status = pool_state.dependency_graph_[tag];
        if (pending_status.tag_provided) {
          --state->remains_required_txs_count;
          BOOST_ASSERT(state->remains_required_txs_count != 0ull);
        } else {
          pending_status.dependents[tx->hash] = state;
        }
      }
      if (auto key = ext_key_repo_->get(tx->hash); key.has_value()) {
        sub_engine_->notify(key.value(),
                            ExtrinsicLifecycleEvent::Future(key.value()));
      }
    });

    return outcome::success();
  }

  void TransactionPoolImpl::rollback(PoolState &pool_state,
                                     const Transaction::Hash &tx_hash) {
    if (auto it = pool_state.pending_txs_.find(tx_hash);
        it != pool_state.pending_txs_.end()) {
      // восстанавливаем зависимости реквайрементов, которые не тригернулись
      BOOST_ASSERT(pool_state.ready_txs_.find(tx_hash)
                   == pool_state.ready_txs_.end());
      auto state = it->second.lock();
      BOOST_ASSERT(state);
      BOOST_ASSERT(state->tx);
      BOOST_ASSERT(state->remains_required_txs_count != 0);

      for (auto &requirement : state->tx->required_tags) {
        auto &pending_status = pool_state.dependency_graph_[requirement];
        if (!pending_status.tag_provided
            and not pending_status.dependents.contains(tx_hash)) {
          ++state->remains_required_txs_count;
          pending_status.dependents[tx_hash] = state;
        }
      }
    } else {
      if (auto it = pool_state.ready_txs_.find(tx_hash);
          it != pool_state.ready_txs_.end()) {
        ReadyStatus ready_status{std::move(it->second)};
        auto state = std::make_shared<TxReadyState>(std::move(ready_status.tx));

        // перемещаем в пендинг
        pool_state.pending_txs_[state->tx->hash] = state;

        // прописываем ее реквайременты в граф
        for (auto &requirement : state->tx->required_tags) {
          PendingStatus &ps = pool_state.dependency_graph_[requirement];
          if (ps.tag_provided) {
            --state->remains_required_txs_count;
          } else {
            ps.dependents[state->tx->hash] = state;
          }
        }

        // проставляем провайдеры как нот тригеред
        for (auto &provider : state->tx->provided_tags) {
          PendingStatus &ps = pool_state.dependency_graph_[provider];
          BOOST_ASSERT(ps.tag_provided);
          ps.tag_provided = false;
        }

        // вызываем для каждого child rollback
        for (auto &h : ready_status.triggered) {
          rollback(pool_state, h);
        }
        pool_state.ready_txs_.erase(it);
      }
    }
    if (auto key = ext_key_repo_->get(tx_hash); key.has_value()) {
      sub_engine_->notify(key.value(),
                          ExtrinsicLifecycleEvent::Future(key.value()));
    }
  }

  outcome::result<Transaction> TransactionPoolImpl::removeOne(
      const Transaction::Hash &tx_hash) {
    return pool_state_.exclusiveAccess(
        [&](auto &pool_state) -> outcome::result<Transaction> {
          if (auto it = pool_state.pending_txs_.find(tx_hash);
              it != pool_state.pending_txs_.end()) {
            BOOST_ASSERT(pool_state.ready_txs_.find(tx_hash)
                         == pool_state.ready_txs_.end());

            auto state = it->second.lock();
            BOOST_ASSERT(state);
            BOOST_ASSERT(state->tx);
            for (auto &tag : state->tx->required_tags) {
              pool_state.dependency_graph_[tag].dependents.erase(tx_hash);
            }

            pool_state.pending_txs_.erase(it);
            return std::move(*state->tx);
          }

          if (auto it = pool_state.ready_txs_.find(tx_hash);
              it != pool_state.ready_txs_.end()) {
            ReadyStatus &ready_status = it->second;
            for (auto &provider : ready_status.tx->provided_tags) {
              PendingStatus &ps = pool_state.dependency_graph_[provider];
              // TODO(kamilsa): Uncomment when #1786 is fixed
              // https://github.com/qdrvm/kagome/issues/1786
              // BOOST_ASSERT(ps.tag_provided);
              ps.tag_provided = false;
            }

            // call rollback for every child
            for (auto &h : ready_status.triggered) {
              rollback(pool_state, h);
            }

            auto t = ready_status.tx;
            pool_state.ready_txs_.erase(it);

            BOOST_ASSERT(t);
            return std::move(*t);
          }

          SL_TRACE(
              logger_,
              "Extrinsic with hash {} was not found in the pool during remove",
              tx_hash);
          return TransactionPoolError::TX_NOT_FOUND;
        });
  }

  void TransactionPoolImpl::getReadyTransactions(
      TxRequestCallback &&callback) const {
    return pool_state_.sharedAccess([&](const auto &pool_state) {
      for (const auto &[_, ready_status] : pool_state.ready_txs_) {
        BOOST_ASSERT(ready_status.tx);
        callback(ready_status.tx);
      }
    });
  }

  std::vector<std::pair<Transaction::Hash, std::shared_ptr<const Transaction>>>
  TransactionPoolImpl::getReadyTransactions() const {
    std::vector<
        std::pair<Transaction::Hash, std::shared_ptr<const Transaction>>>
        txs;
    getReadyTransactions([&](const auto &tx) {
      txs.emplace_back(std::make_pair(tx->hash, tx));
    });
    return txs;
  }

  void TransactionPoolImpl::getPendingTransactions(
      TxRequestCallback &&callback) const {
    return pool_state_.sharedAccess([&](const auto &pool_state) {
      for (const auto &[_, wptr] : pool_state.pending_txs_) {
        auto tx_ready_state = wptr.lock();
        BOOST_ASSERT(tx_ready_state);

        callback(tx_ready_state->tx);
      }
    });
  }

  outcome::result<std::vector<Transaction>> TransactionPoolImpl::removeStale(
      const primitives::BlockId &at) {
    OUTCOME_TRY(number, header_repo_->getNumberById(at));

    std::vector<Transaction::Hash> remove_to;
    pool_state_.exclusiveAccess([&](auto &pool_state) {
      for (const auto &[txHash, ready_status] : pool_state.ready_txs_) {
        BOOST_ASSERT(ready_status.tx);
        if (moderator_->banIfStale(number, *ready_status.tx)) {
          remove_to.emplace_back(txHash);
        }
      }
      for (const auto &[txHash, wp_tx_ready_state] : pool_state.pending_txs_) {
        auto tx_ready_state = wp_tx_ready_state.lock();
        BOOST_ASSERT(tx_ready_state);
        BOOST_ASSERT(tx_ready_state->tx);
        if (moderator_->banIfStale(number, *tx_ready_state->tx)) {
          remove_to.emplace_back(txHash);
        }
      }
    });

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

  void TransactionPoolImpl::setReady(PoolState &pool_state,
                                     const std::shared_ptr<Transaction> &tx) {
    if (auto [it, ok] =
            pool_state.ready_txs_.emplace(tx->hash, ReadyStatus{.tx = tx});
        ok) {
      if (auto key = ext_key_repo_->get(tx->hash); key.has_value()) {
        sub_engine_->notify(key.value(),
                            ExtrinsicLifecycleEvent::Ready(key.value()));
      }

      for (const auto &tag : tx->provided_tags) {
        PendingStatus &status = pool_state.dependency_graph_[tag];
        status.tag_provided = true;

        for (auto &dep : status.dependents) {
          auto dependent = std::move(dep.second);
          if (dependent) {
            BOOST_ASSERT(dependent->tx);
            it->second.triggered.emplace_back(dependent->tx->hash);
            if (--dependent->remains_required_txs_count == 0ull) {
              pool_state.pending_txs_.erase(dependent->tx->hash);
              setReady(pool_state, dependent->tx);
            }
          }
        }
        status.dependents.clear();
      }
      metric_ready_txs_->set(pool_state.ready_txs_.size());
    }
  }

  TransactionPoolImpl::Status TransactionPoolImpl::getStatus() const {
    return pool_state_.sharedAccess([&](const auto &pool_state) {
      return Status{pool_state.ready_txs_.size(),
                    pool_state.pending_txs_.size()};
    });
  }

}  // namespace kagome::transaction_pool
