/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <ec-cpp/errors.hpp>
#include "outcome/outcome.hpp"

namespace kagome {
  /**
   * #include <erasure_coding/erasure_coding.h>
   * enum NPRSResult_Tag;
   */
  enum class ErasureCodingError {};

  enum class ErasureCodingRootError {
    MISMATCH = 1,
  };

  constexpr int kErrorOffset = 0x01000000;
  inline ErasureCodingError toErasureCodingError(ec_cpp::Error code) {
    return ErasureCodingError(kErrorOffset + (int)code);
  }
  inline ErasureCodingError fromErasureCodingError(ErasureCodingError code) {
    return ErasureCodingError((int)code - kErrorOffset);
  }

}  // namespace kagome

OUTCOME_HPP_DECLARE_ERROR(kagome, ErasureCodingError)
OUTCOME_HPP_DECLARE_ERROR(kagome, ErasureCodingRootError)
