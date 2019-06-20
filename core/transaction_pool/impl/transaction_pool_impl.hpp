/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSACTION_POOL_IMPL_HPP
#define KAGOME_TRANSACTION_POOL_IMPL_HPP

#include <outcome/outcome.hpp>
#include "common/logger.hpp"
#include "storage/face/generic_list.hpp"
#include "transaction_pool/pool_moderator.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::transaction_pool {

  struct ReadyTransaction : public primitives::Transaction {};

  struct WaitingTransaction : public primitives::Transaction {
    std::vector<primitives::TransactionTag> still_requires;
  };

  class TransactionPoolImpl : public TransactionPool {
    template <typename T>
    using Container = face::GenericList<T>;
    static constexpr auto kDefaultLoggerTag = "Transaction Pool: ";
    static constexpr size_t kDefaultMaxReadyNum = 512;
    static constexpr size_t kDefaultMaxWaitingNum = 128;

   public:
    enum class Error { ALREADY_IMPORTED = 1 };

    template <template <typename T> typename Container>
    static auto create(
        std::unique_ptr<PoolModerator> moderator,
        Limits limits = Limits{kDefaultMaxReadyNum, kDefaultMaxWaitingNum},
        common::Logger logger = common::createLogger(kDefaultLoggerTag)) {
      return std::make_shared<TransactionPoolImpl>(TransactionPoolImpl{
          std::move(moderator), std::make_unique<Container<ReadyTransaction>>(),
          std::make_unique<Container<WaitingTransaction>>(), limits,
          std::move(logger)});
    }

    TransactionPoolImpl(TransactionPoolImpl &&) = default;
    TransactionPoolImpl(const TransactionPoolImpl &&) = delete;

    ~TransactionPoolImpl() override = default;

    TransactionPool &operator=(const TransactionPool &) = delete;

    outcome::result<void> submitOne(primitives::Transaction t) override;

    outcome::result<void> submit(
        std::vector<primitives::Transaction> ts) override;

    std::vector<primitives::Transaction> getReadyTransactions() override;

    std::vector<primitives::Transaction> removeStale(
        const primitives::BlockId &at) override;

    std::vector<primitives::Transaction> prune(
        const std::vector<primitives::Extrinsic> &exts) override;

    std::vector<primitives::Transaction> pruneTags(
        const std::vector<primitives::TransactionTag> &tag) override;

    Status getStatus() const override;

   private:
    TransactionPoolImpl(std::unique_ptr<PoolModerator> moderator,
                        std::unique_ptr<Container<ReadyTransaction>> ready,
                        std::unique_ptr<Container<WaitingTransaction>> waiting,
                        Limits limits, common::Logger logger);
    /**
     * If there are waiting transactions with satisfied tag requirements, move
     * them to ready queue
     */
    void updateReady();

    std::vector<primitives::Transaction> enforceLimits();

    common::Logger logger_;

    // bans stale and invalid transactions for some amount of time
    std::unique_ptr<PoolModerator> moderator_;

    // transactions with satisfied dependencies ready to leave the pool
    std::unique_ptr<Container<ReadyTransaction>> ready_queue_;

    // transactions waiting for their dependencies to resolve
    std::unique_ptr<Container<WaitingTransaction>> waiting_queue_;

    // tags provided by imported transactions
    std::set<primitives::TransactionTag> provided_tags_;

    // hexadecimal representations of imported transactions hashes
    // optimization of searching for already inserted transactions
    std::set<std::string> imported_hashes_;

    Limits limits_;
  };

}  // namespace kagome::transaction_pool

OUTCOME_HPP_DECLARE_ERROR(kagome::transaction_pool, TransactionPoolImpl::Error);

#endif  // KAGOME_TRANSACTION_POOL_IMPL_HPP
