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

    outcome::result<void> submit_at(primitives::BlockId at, Transaction t);

    std::vector<Transaction> getReadyTransactions();
  };
}

#endif  // KAGOME_TRANSACTION_POOL_HPP
