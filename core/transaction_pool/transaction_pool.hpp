/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSACTION_POOL_HPP
#define KAGOME_TRANSACTION_POOL_HPP

#include <outcome/outcome.hpp>

#include "primitives/block_id.hpp"
#include "primitives/transaction.hpp"
#include "primitives/transaction_validity.hpp"

namespace kagome::transaction_pool {

  using primitives::Transaction;

  class TransactionPool {
   public:
    struct Status;
    struct Limits;
    using TxRequestCallback =
        std::function<void(const std::shared_ptr<const Transaction> &)>;

    virtual ~TransactionPool() = default;

    /**
     * @return pending transactions
     */
    virtual void getPendingTransactions(TxRequestCallback &&callback) const = 0;

    /**
     * Builds and validates transaction for provided extrinsic, and submit
     * result transaction into pool
     * @param source how extrinsic was received (for example external or
     * submitted through offchain worker)
     * @param extrinsic set of bytes representing either transaction or inherent
     * @return hash of successfully submitted transaction
     * or error if state is invalid or unknown
     */
    virtual outcome::result<Transaction::Hash> submitExtrinsic(
        primitives::TransactionSource source,
        primitives::Extrinsic extrinsic) = 0;

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
     * Remove transaction from the pool
     * @param txHash - hash of the removed transaction
     * @returns removed transaction or error
     */
    virtual outcome::result<Transaction> removeOne(
        const Transaction::Hash &txHash) = 0;

    /**
     * @return transactions ready to included in the next block
     */
    virtual void getReadyTransactions(TxRequestCallback &&callback) const = 0;

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

    virtual outcome::result<primitives::Transaction> constructTransaction(
        primitives::TransactionSource source,
        primitives::Extrinsic extrinsic) const = 0;
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
