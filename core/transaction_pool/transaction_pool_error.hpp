/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_TRANSACTION_POOL_TRANSACTION_POOL_ERROR_HPP
#define KAGOME_CORE_TRANSACTION_POOL_TRANSACTION_POOL_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::transaction_pool {
  enum class TransactionPoolError { ALREADY_IMPORTED = 1 };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::transaction_pool, TransactionPoolError);

#endif  // KAGOME_CORE_TRANSACTION_POOL_TRANSACTION_POOL_ERROR_HPP
