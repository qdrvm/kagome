/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>

namespace kagome::network {
  /**
   * @brief interface adapters errors
   */
  enum class AdaptersError : int {
    EMPTY_DATA = 1,
    DATA_SIZE_CORRUPTED,
    PARSE_FAILED,
    UNEXPECTED_VARIANT,
    CAST_FAILED,
  };

}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, AdaptersError)
