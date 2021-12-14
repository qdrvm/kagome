/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_ERROR_HPP
#define KAGOME_BABE_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::consensus {
  enum class BabeError {
    TIMER_ERROR = 1,
    NODE_FALL_BEHIND,
    MISSING_PROOF,
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, BabeError)

#endif  // KAGOME_BABE_ERROR_HPP
