/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>

namespace kagome::consensus::babe {

  enum class BabeError {
    CAN_NOT_PREPARE_BLOCK = 1,
    CAN_NOT_PROPOSE_BLOCK,
    CAN_NOT_SEAL_BLOCK,
    WAS_NOT_BUILD_ON_TIME,
    CAN_NOT_SAVE_BLOCK,
    MISSING_PROOF,
    BAD_ORDER_OF_DIGEST_ITEM,
    UNKNOWN_DIGEST_TYPE,
    SLOT_BEFORE_GENESIS,
  };

}  // namespace kagome::consensus::babe

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::babe, BabeError)
