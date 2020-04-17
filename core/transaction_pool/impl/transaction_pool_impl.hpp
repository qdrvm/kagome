/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSACTION_POOL_IMPL_HPP
#define KAGOME_TRANSACTION_POOL_IMPL_HPP

#include <outcome/outcome.hpp>
#include "blockchain/block_header_repository.hpp"
#include "common/logger.hpp"
#include "transaction_pool/pool_moderator.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::transaction_pool {

  class TransactionPoolImpl : public TransactionPool {
    static constexpr auto kDefaultLoggerTag = "Transaction Pool";

   public:
    TransactionPoolImpl(
        std::unique_ptr<PoolModerator> moderator,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
        Limits limits);

    TransactionPoolImpl(TransactionPoolImpl &&) = default;
    TransactionPoolImpl(const TransactionPoolImpl &) = delete;

    ~TransactionPoolImpl() override = default;

    TransactionPoolImpl &operator=(TransactionPoolImpl &&) = delete;
    TransactionPoolImpl &operator=(const TransactionPoolImpl &) = delete;

    outcome::result<void> submitOne(Transaction &&tx) override;
    outcome::result<void> submit(std::vector<Transaction> txs) override;

    outcome::result<void> removeOne(const Transaction::Hash &tx_hash) override;
    outcome::result<void> remove(
        const std::vector<Transaction::Hash> &tx_hashes) override;

    std::vector<Transaction> getReadyTransactions() override;

    outcome::result<std::vector<Transaction>> removeStale(
        const primitives::BlockId &at) override;

    Status getStatus() const override;

   private:
    outcome::result<void> submitOne(const std::shared_ptr<Transaction> &tx);

    outcome::result<void> processTransaction(
        const std::shared_ptr<Transaction> &tx);

    outcome::result<void> processTransactionAsReady(
        const std::shared_ptr<Transaction> &tx);

    outcome::result<void> processTransactionAsWaiting(
        const std::shared_ptr<Transaction> &tx);

    outcome::result<void> ensureSpace() const;

    bool hasSpaceInReady() const;

    void addTransactionAsWaiting(const std::shared_ptr<Transaction> &tx);

    void delTransactionAsWaiting(const std::shared_ptr<Transaction> &tx);

    /// Postpone ready transaction (in case ready limit was enreach before)
    void postponeTransaction(const std::shared_ptr<Transaction> &tx);

    /// Process postponed transactions (in case appearing space for them)
    void processPostponedTransactions();

    void provideTag(const Transaction::Tag &tag);

    void unprovideTag(const Transaction::Tag &tag);

    void commitRequiredTags(const std::shared_ptr<Transaction> &tx);

    void commitProvidedTags(const std::shared_ptr<Transaction> &tx);

    void rollbackRequiredTags(const std::shared_ptr<Transaction> &tx);

    void rollbackProvidedTags(const std::shared_ptr<Transaction> &tx);

    bool checkForReady(const std::shared_ptr<const Transaction> &tx) const;

    void setReady(const std::shared_ptr<Transaction> &tx);

    void unsetReady(const std::shared_ptr<Transaction> &tx);

    bool isInReady(const std::shared_ptr<const Transaction> &tx) const;

    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;

    common::Logger logger_ = common::createLogger(kDefaultLoggerTag);

    // bans stale and invalid transactions for some amount of time
    std::unique_ptr<PoolModerator> moderator_;

    /// All of imported transaction, contained in the pool
    std::unordered_map<Transaction::Hash, std::shared_ptr<Transaction>>
        imported_txs_;

    /// Collection transaction with full-satisfied dependensies
    std::unordered_map<Transaction::Hash, std::weak_ptr<Transaction>>
        ready_txs_;

    /// List of ready transaction over limit. It will be process first of all
    std::list<std::weak_ptr<Transaction>> postponed_txs_;

    /// Transactions which provides specific tags
    std::multimap<Transaction::Tag, std::weak_ptr<Transaction>>
        tx_provides_tag_;

    /// Transactions with resolved requirement of a specific tag
    std::multimap<Transaction::Tag, std::weak_ptr<Transaction>>
        tx_depends_on_tag_;

    /// Transactions with unresolved require of specific tags
    std::multimap<Transaction::Tag, std::weak_ptr<Transaction>> tx_waits_tag_;

    Limits limits_;
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TRANSACTION_POOL_IMPL_HPP
