/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>

namespace kagome::transaction_pool {
  enum class TransactionPoolError {
    TX_ALREADY_IMPORTED = 1,
    TX_NOT_FOUND,
    POOL_IS_FULL,
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::transaction_pool, TransactionPoolError);
