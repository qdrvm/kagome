/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_TRANSACTION_POOL_TRANSACTION_POOL_ERROR_HPP
#define KAGOME_CORE_TRANSACTION_POOL_TRANSACTION_POOL_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::transaction_pool {
  enum class TransactionPoolError {
    TX_ALREADY_IMPORTED = 1,
    TX_NOT_FOUND,
    POOL_IS_FULL,
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::transaction_pool, TransactionPoolError);

#endif  // KAGOME_CORE_TRANSACTION_POOL_TRANSACTION_POOL_ERROR_HPP
