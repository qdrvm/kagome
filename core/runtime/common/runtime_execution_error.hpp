/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>

namespace kagome::runtime {

  /**
   * @brief RuntimeExecutionError enum provides error codes for storage
   * transactions mechanism
   */
  enum class RuntimeExecutionError {  // 0 is reserved for success
    NO_TRANSACTIONS_WERE_STARTED = 1,
    EXPORT_FUNCTION_NOT_FOUND
  };
}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, RuntimeExecutionError);
