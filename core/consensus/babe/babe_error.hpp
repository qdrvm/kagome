/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_BABEERROR
#define KAGOME_CONSENSUS_BABE_BABEERROR

#include <outcome/outcome.hpp>

namespace kagome::consensus::babe {

  enum class BabeError {
    MISSING_PROOF = 1,
    BAD_ORDER_OF_DIGEST_ITEM,
    UNKNOWN_DIGEST_TYPE,
    SLOT_BEFORE_GENESIS,
  };

}  // namespace kagome::consensus::babe

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::babe, BabeError)

#endif  // KAGOME_CONSENSUS_BABE_BABEERROR
