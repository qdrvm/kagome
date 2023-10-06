/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_COMMON_RUNTIME_TRANSACTION_ERROR_HPP
#define KAGOME_CORE_RUNTIME_COMMON_RUNTIME_TRANSACTION_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::runtime {

  /**
   * @brief RuntimeTransactionError enum provides error codes for storage
   * transactions mechanism
   */
  enum class RuntimeTransactionError {  // 0 is reserved for success
    NO_TRANSACTIONS_WERE_STARTED = 1,
    EXPORT_FUNCTION_NOT_FOUND,
  };
}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, RuntimeTransactionError);

#endif  // KAGOME_CORE_RUNTIME_COMMON_RUNTIME_TRANSACTION_ERROR_HPP
