/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSACTION_POOL_IMPL_HPP
#define KAGOME_TRANSACTION_POOL_IMPL_HPP

#include "storage/face/generic_list.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::transaction_pool {

  class TransactionPoolImpl : public TransactionPool {
    using Container = face::GenericList<primitives::Transaction>;

   public:
    explicit TransactionPoolImpl(std::unique_ptr<Container> ready,
                                 std::unique_ptr<Container> waiting);

    ~TransactionPoolImpl() override = default;

    outcome::result<void> submitOne(primitives::Transaction t) override;

    outcome::result<void> submit(
        std::vector<primitives::Transaction> ts) override;

    std::vector<primitives::Transaction> getReadyTransactions() override;

    std::vector<primitives::Transaction> removeStale(
        const primitives::BlockId &at) override;

    std::vector<primitives::Transaction> pruneByTag(
        primitives::TransactionTag tag) override;

    Status getStatus() const override;

   private:
    /**
     * If there are waiting transactions with satisfied tag requirements, move
     * them to ready queue
     */
    void updateReady();

    std::unique_ptr<Container> ready_queue_;
    std::unique_ptr<Container> waiting_queue_;
    std::set<primitives::TransactionTag> provided_tags_;
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TRANSACTION_POOL_IMPL_HPP
