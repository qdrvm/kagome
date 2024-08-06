/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <deque>
#include <libp2p/common/byteutil.hpp>

#include "blockchain/block_header_repository.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "outcome/outcome.hpp"
#include "primitives/event_types.hpp"
#include "primitives/transaction_validity.hpp"
#include "subscription/extrinsic_event_key_repository.hpp"
#include "transaction_pool/pool_moderator.hpp"
#include "transaction_pool/transaction_pool.hpp"
#include "utils/safe_object.hpp"

namespace kagome::runtime {
  class TaggedTransactionQueue;
}
namespace kagome::crypto {
  class Hasher;
}
namespace kagome::network {
  class TransactionsTransmitter;
}

namespace kagome::transaction_pool {

  class TransactionPoolImpl : public TransactionPool {
   public:
    TransactionPoolImpl(
        std::shared_ptr<runtime::TaggedTransactionQueue> ttq,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<network::TransactionsTransmitter> tx_transmitter,
        std::unique_ptr<PoolModerator> moderator,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
            sub_engine,
        std::shared_ptr<subscription::ExtrinsicEventKeyRepository> ext_key_repo,
        Limits limits);

    TransactionPoolImpl(TransactionPoolImpl &&) = default;
    TransactionPoolImpl(const TransactionPoolImpl &) = delete;

    ~TransactionPoolImpl() override = default;

    TransactionPoolImpl &operator=(TransactionPoolImpl &&) = delete;
    TransactionPoolImpl &operator=(const TransactionPoolImpl &) = delete;

    void getPendingTransactions(TxRequestCallback &&callback) const override;

    outcome::result<Transaction::Hash> submitExtrinsic(
        primitives::TransactionSource source,
        primitives::Extrinsic extrinsic) override;

    outcome::result<void> submitOne(Transaction &&tx) override;

    outcome::result<Transaction> removeOne(
        const Transaction::Hash &tx_hash) override;

    void getReadyTransactions(TxRequestCallback &&callback) const override;
    std::vector<
        std::pair<Transaction::Hash, std::shared_ptr<const Transaction>>>
    getReadyTransactions() const override;

    outcome::result<std::vector<Transaction>> removeStale(
        const primitives::BlockId &at) override;

    Status getStatus() const override;

    outcome::result<primitives::Transaction> constructTransaction(
        primitives::TransactionSource source,
        primitives::Extrinsic extrinsic) const override;

   private:
    struct TxReadyState {
      uint32_t remains_required_txs_count;
      std::shared_ptr<Transaction> tx;

      explicit TxReadyState(Transaction &&other)
          : remains_required_txs_count{(uint32_t)other.required_tags.size()},
            tx{std::make_shared<Transaction>(std::move(other))} {}
      explicit TxReadyState(const std::shared_ptr<Transaction> &other)
          : remains_required_txs_count{(uint32_t)other->required_tags.size()},
            tx{other} {}

      TxReadyState(const TxReadyState &) = delete;
      TxReadyState(TxReadyState &&other)
          : remains_required_txs_count{other.remains_required_txs_count},
            tx{std::move(other.tx)} {}

      TxReadyState &operator=(const TxReadyState &) = delete;
      TxReadyState &operator=(TxReadyState &&other) {
        if (this != &other) {
          remains_required_txs_count = other.remains_required_txs_count;
          tx = std::move(other.tx);
        }
        return *this;
      }
    };

    struct PendingStatus {
      bool tag_provided{false};
      std::unordered_map<Transaction::Hash, std::shared_ptr<TxReadyState>>
          dependents{};
    };

    struct ReadyStatus {
      std::shared_ptr<Transaction> tx;
      std::deque<Transaction::Hash> triggered;
    };

    struct PoolState {
      /// pendings
      std::unordered_map<Transaction::Tag, PendingStatus> dependency_graph_;
      std::unordered_map<Transaction::Hash, std::weak_ptr<TxReadyState>>
          pending_txs_;

      /// Collection transaction with full-satisfied dependencies
      std::unordered_map<Transaction::Hash, ReadyStatus> ready_txs_;
    };

    bool imported(const Transaction::Hash &tx_hash) const;
    bool is_ready(const PoolState &pool_state,
                  const std::shared_ptr<const Transaction> &tx) const;
    size_t imported_txs_count() const;

    void rollback(PoolState &pool_state, const Transaction::Hash &tx_hash);

    outcome::result<void> submitOneInternal(
        const std::shared_ptr<Transaction> &tx);

    outcome::result<void> processTransaction(
        const std::shared_ptr<Transaction> &tx);

    void setReady(PoolState &pool_state,
                  const std::shared_ptr<Transaction> &tx);

    outcome::result<Transaction> constructTransaction(
        primitives::TransactionSource source,
        primitives::Extrinsic extrinsic,
        const Transaction::Hash &extrinsic_hash) const;

    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;

    log::Logger logger_ = log::createLogger("TransactionPool", "transactions");

    std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
        sub_engine_;
    std::shared_ptr<subscription::ExtrinsicEventKeyRepository> ext_key_repo_;

    std::shared_ptr<runtime::TaggedTransactionQueue> ttq_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<network::TransactionsTransmitter> tx_transmitter_;

    /// bans stale and invalid transactions for some amount of time
    std::unique_ptr<PoolModerator> moderator_;

    SafeObject<PoolState> pool_state_;
    Limits limits_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_ready_txs_;
  };

}  // namespace kagome::transaction_pool
