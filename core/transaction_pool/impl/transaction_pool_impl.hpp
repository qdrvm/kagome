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

#ifndef KAGOME_TRANSACTION_POOL_IMPL_HPP
#define KAGOME_TRANSACTION_POOL_IMPL_HPP

#include <outcome/outcome.hpp>
#include "blockchain/block_header_repository.hpp"
#include "common/logger.hpp"
#include "transaction_pool/pool_moderator.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::transaction_pool {

  struct ReadyTransaction : public primitives::Transaction {
    std::list<common::Hash256> unlocks;
  };

  struct WaitingTransaction : public primitives::Transaction {};

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

    outcome::result<void> submitOne(primitives::Transaction t) override;

    outcome::result<void> submit(
        std::vector<primitives::Transaction> ts) override;

    std::vector<primitives::Transaction> getReadyTransactions() override;

    outcome::result<std::vector<primitives::Transaction>> removeStale(
        const primitives::BlockId &at) override;

    Status getStatus() const override;

    std::vector<primitives::Transaction> pruneTag(
        const primitives::BlockId &at,
        const primitives::TransactionTag &tag,
        const std::vector<common::Hash256> &known_imported_hashes) override;

    std::vector<primitives::Transaction> pruneTag(
        const primitives::BlockId &at,
        const primitives::TransactionTag &tag) override;

   private:
    /**
     * If there are waiting transactions with satisfied tag requirements, move
     * them to ready queue
     */
    void updateReady();
    bool isReady(const WaitingTransaction &tx) const;

    /**
     * When a transaction becomes ready, add it to 'unlocks' list of
     * corresponding transactions
     */
    void updateUnlockingTransactions(const ReadyTransaction &rtx);

    std::vector<primitives::Transaction> enforceLimits();

    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;

    common::Logger logger_ = common::createLogger(kDefaultLoggerTag);

    // bans stale and invalid transactions for some amount of time
    std::unique_ptr<PoolModerator> moderator_;

    // transactions with satisfied dependencies ready to leave the pool
    std::map<common::Hash256, ReadyTransaction> ready_queue_;

    // transactions waiting for their dependencies to resolve
    std::list<WaitingTransaction> waiting_queue_;

    // tags provided by imported transactions
    std::map<primitives::TransactionTag, boost::optional<common::Hash256>>
        provided_tags_by_;

    // hexadecimal representations of imported transactions hashes
    // optimization of searching for already inserted transactions
    std::set<std::string> imported_hashes_;

    Limits limits_;
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TRANSACTION_POOL_IMPL_HPP
