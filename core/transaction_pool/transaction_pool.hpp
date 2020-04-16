/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
    virtual outcome::result<std::vector<primitives::Transaction>> removeStale(
        const primitives::BlockId &at) = 0;

    /* Prunes ready transactions that provide given list of tags.
     *
     * Given tags are assumed to be always provided now, so all transactions
     * in the Future Queue that require that particular tag (and have other
     * requirements satisfied) are promoted to Ready Queue.
     *
     * Moreover for each provided tag we remove transactions in the pool that:
     * 1. Provide that tag directly
     * 2. Are a dependency of pruned transaction.
     * The transactions in \param known_imported_hashes
     * (if pruned) are not revalidated and become temporarily banned to
     * prevent importing them in the (near) future.
     */
    virtual std::vector<primitives::Transaction> pruneTag(
        const primitives::BlockId &at,
        const primitives::TransactionTag &tag,
        const std::vector<common::Hash256> &known_imported_hashes) = 0;

    /**
     * Same as above, but known imported hashes are empty
     */
    virtual std::vector<primitives::Transaction> pruneTag(
        const primitives::BlockId &at,
        const primitives::TransactionTag &tag) = 0;

    virtual Status getStatus() const = 0;
  };

  struct TransactionPool::Status {
    size_t ready_num;
    size_t waiting_num;
  };

  struct TransactionPool::Limits {
    static constexpr size_t kDefaultMaxReadyNum = 128;
    static constexpr size_t kDefaultMaxWaitingNum = 512;

    size_t max_ready_num = kDefaultMaxReadyNum;
    size_t max_waiting_num = kDefaultMaxWaitingNum;
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TRANSACTION_POOL_HPP
