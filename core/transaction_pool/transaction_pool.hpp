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

  class TransactionPool {
   public:
    struct Status;
    struct Limits;

    virtual ~TransactionPool() = default;

    /**
     * Import one verified transaction to the pool. If it has unresolved
     * dependencies (requires tags of transactions that are not in the pool
     * yet), it will wait in the pool until its dependencies are solved, in
     * which case it becomes ready and may be pruned, or it is banned from the
     * pool for some amount of time as its longevity is reached or the pool is
     * overflown
     */
    virtual outcome::result<void> submitOne(primitives::Transaction t) = 0;

    /**
     * Import several transactions to the pool
     * @see submitOne()
     */
    virtual outcome::result<void> submit(
        std::vector<primitives::Transaction> ts) = 0;

    /**
     * @return transactions ready to included in the next block, sorted by their
     * priority
     */
    virtual std::vector<primitives::Transaction> getReadyTransactions() = 0;

    /**
     * Remove from the pool and temporarily ban transactions which longevity is
     * expired
     * @param at a block that is considered current for removal (transaction t
     * is banned if
     * 'block number when t got to pool' + 't.longevity' <= block number of at)
     * @return removed transactions
     */
    virtual std::vector<primitives::Transaction> removeStale(
        const primitives::BlockId &at) = 0;

    virtual std::vector<primitives::Transaction> prune(
        const std::vector<primitives::Extrinsic> &exts) = 0;

    virtual std::vector<primitives::Transaction> pruneTags(
        const std::vector<primitives::TransactionTag> &tag) = 0;

    virtual Status getStatus() const = 0;
  };

  struct TransactionPool::Status {
    size_t ready_num{};
    size_t waiting_num{};
  };

  struct TransactionPool::Limits {
    size_t max_ready_num{};
    size_t max_waiting_num{};
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TRANSACTION_POOL_HPP
