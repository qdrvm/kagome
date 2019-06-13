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

    virtual ~TransactionPool() = default;

    virtual outcome::result<void> submitOne(primitives::Transaction t) = 0;

    virtual outcome::result<void> submit(
        std::vector<primitives::Transaction> ts) = 0;

    virtual std::vector<primitives::Transaction> getReadyTransactions() = 0;

    virtual std::vector<primitives::Transaction> removeStale(
        const primitives::BlockId &at) = 0;

    virtual std::vector<primitives::Transaction> pruneByTag(
        primitives::TransactionTag tag) = 0;

    virtual Status getStatus() const = 0;
  };

  struct TransactionPool::Status {
   public:
    Status &ready(size_t n) {
      ready_num = n;
      return *this;
    }
    Status &waiting(size_t n) {
      waiting_num = n;
      return *this;
    }

    size_t ready() const {
      return ready_num;
    }

    size_t waiting() const {
      return waiting_num;
    }

   private:
    size_t ready_num;
    size_t waiting_num;
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TRANSACTION_POOL_HPP
