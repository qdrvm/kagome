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
    virtual ~TransactionPool() = default;

    virtual outcome::result<void> submitAt(primitives::BlockId at,
                                           primitives::Transaction t) = 0;

    virtual outcome::result<void> submitAt(
        primitives::BlockId at, std::vector<primitives::Transaction> t) = 0;

    virtual std::vector<primitives::Transaction> getReadyTransactions() = 0;

    virtual std::vector<primitives::Transaction> removeStale() = 0;

    virtual std::vector<primitives::Transaction> pruneByTag(primitives::TransactionTag tag) = 0;
  };
}  // namespace kagome::transaction_pool

#endif  // KAGOME_TRANSACTION_POOL_HPP
