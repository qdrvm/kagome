/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSACTION_POOL_HPP
#define KAGOME_TRANSACTION_POOL_HPP

#include <outcome/outcome.hpp>

#include "primitives/block_id.hpp"
#include "primitives/transaction.hpp"

namespace kagome::transaction_pool {

  using primitives::Transaction;

  class TransactionPool {
   public:
    struct Status;
    struct Limits;

    virtual ~TransactionPool() = default;

    /**
     * @return pending transactions
     */
    virtual const std::unordered_map<Transaction::Hash,
                                     std::shared_ptr<Transaction>>
        &getPendingTransactions() const = 0;

    /**
     * Import one verified transaction to the pool. If it has unresolved
     * dependencies (requires tags of transactions that are not in the pool
     * yet), it will wait in the pool until its dependencies are solved, in
     * which case it becomes ready and may be pruned, or it is banned from the
     * pool for some amount of time as its longevity is reached or the pool is
     * overflown
     */
    virtual outcome::result<void> submitOne(Transaction &&tx) = 0;

    /**
     * Import several transactions to the pool
     * @see submitOne()
     */
    virtual outcome::result<void> submit(std::vector<Transaction> txs) = 0;

    /**
     * Remove transaction from the pool
     * @param txHash - hash of the removed transaction
     * @returns removed transaction or error
     */
    virtual outcome::result<Transaction> removeOne(
        const Transaction::Hash &txHash) = 0;

    /**
     * Trying to remove several transactions from the pool
     * @see removeOne()
     */
    virtual void remove(const std::vector<Transaction::Hash> &txHashes) = 0;

    /**
     * @return transactions ready to included in the next block, sorted by their
     * priority
     */
    virtual std::map<Transaction::Hash, std::shared_ptr<Transaction>>
    getReadyTransactions() const = 0;

    /**
     * Remove from the pool and temporarily ban transactions which longevity is
     * expired
     * @param at a block that is considered current for removal (transaction t
     * is banned if
     * 'block number when t got to pool' + 't.longevity' <= block number of at)
     * @return removed transactions
     */
    virtual outcome::result<std::vector<Transaction>> removeStale(
        const primitives::BlockId &at) = 0;

    virtual Status getStatus() const = 0;
  };

  struct TransactionPool::Status {
    size_t ready_num;
    size_t waiting_num;
  };

  struct TransactionPool::Limits {
    static constexpr size_t kDefaultMaxReadyNum = 128;
    static constexpr size_t kDefaultCapacity = 512;

    size_t max_ready_num = kDefaultMaxReadyNum;
    size_t capacity = kDefaultCapacity;
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TRANSACTION_POOL_HPP
