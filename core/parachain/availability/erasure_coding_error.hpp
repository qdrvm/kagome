/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_AVAILABILITY_ERASURE_CODING_ERROR_HPP
#define KAGOME_PARACHAIN_AVAILABILITY_ERASURE_CODING_ERROR_HPP

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
}  // namespace kagome

OUTCOME_HPP_DECLARE_ERROR(kagome, ErasureCodingError)
OUTCOME_HPP_DECLARE_ERROR(kagome, ErasureCodingRootError)

#endif  // KAGOME_PARACHAIN_AVAILABILITY_ERASURE_CODING_ERROR_HPP
