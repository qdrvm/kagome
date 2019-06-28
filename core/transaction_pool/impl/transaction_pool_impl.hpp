/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSACTION_POOL_IMPL_HPP
#define KAGOME_TRANSACTION_POOL_IMPL_HPP

#include <outcome/outcome.hpp>
#include "common/logger.hpp"
#include "transaction_pool/pool_moderator.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::transaction_pool {

  struct ReadyTransaction : public primitives::Transaction {
    std::list<common::Hash256> unlocks;
  };

  struct WaitingTransaction : public primitives::Transaction {};

  class TransactionPoolImpl : public TransactionPool {
    static constexpr auto kDefaultLoggerTag = "Transaction Pool: ";
    static constexpr size_t kDefaultMaxReadyNum = 512;
    static constexpr size_t kDefaultMaxWaitingNum = 128;

   public:
    template <template <typename T> typename Container>
    static auto create(
        std::unique_ptr<PoolModerator> moderator,
        Limits limits = Limits{kDefaultMaxReadyNum, kDefaultMaxWaitingNum},
        common::Logger logger = common::createLogger(kDefaultLoggerTag)) {
      return std::make_shared<TransactionPoolImpl>(
          TransactionPoolImpl{std::move(moderator), limits, std::move(logger)});
    }

    TransactionPoolImpl(TransactionPoolImpl &&) = default;
    TransactionPoolImpl(const TransactionPoolImpl &) = delete;

    ~TransactionPoolImpl() override = default;

    TransactionPoolImpl &operator=(TransactionPoolImpl &&) = delete;
    TransactionPoolImpl &operator=(const TransactionPoolImpl &) = delete;

    outcome::result<void> submitOne(primitives::Transaction t) override;

    outcome::result<void> submit(
        std::vector<primitives::Transaction> ts) override;

    std::vector<primitives::Transaction> getReadyTransactions() override;

    std::vector<primitives::Transaction> removeStale(
        const primitives::BlockId &at) override;

    Status getStatus() const override;

    std::vector<primitives::Transaction> pruneTag(
        const primitives::BlockId &at, const primitives::TransactionTag &tag,
        const std::vector<common::Hash256> &known_imported_hashes) override;

   private:
    TransactionPoolImpl(std::unique_ptr<PoolModerator> moderator, Limits limits,
                        common::Logger logger);
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

    common::Logger logger_;

    // bans stale and invalid transactions for some amount of time
    std::unique_ptr<PoolModerator> moderator_;

    // transactions with satisfied dependencies ready to leave the pool
    std::map<common::Hash256, ReadyTransaction> ready_queue_;

    // transactions waiting for their dependencies to resolve
    std::list<WaitingTransaction> waiting_queue_;

    // tags provided by imported transactions
    std::map<primitives::TransactionTag, std::optional<common::Hash256>>
        provided_tags_by_;

    // hexadecimal representations of imported transactions hashes
    // optimization of searching for already inserted transactions
    std::set<std::string> imported_hashes_;

    Limits limits_;
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TRANSACTION_POOL_IMPL_HPP
