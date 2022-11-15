/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_ERROR_HPP
#define KAGOME_BABE_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::consensus::babe {
  enum class BabeError {
    TIMER_ERROR = 1,
    NODE_FALL_BEHIND,
    MISSING_PROOF,
    BAD_ORDER_OF_DIGEST_ITEM,
    UNKNOWN_DIGEST_TYPE,
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::babe, BabeError)

#endif  // KAGOME_BABE_ERROR_HPP
