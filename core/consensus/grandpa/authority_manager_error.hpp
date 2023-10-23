/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/outcome.hpp"

namespace kagome::consensus::grandpa {
  enum class AuthorityManagerError {
    UNKNOWN_ENGINE_ID = 1,
    ORPHAN_BLOCK_OR_ALREADY_FINALIZED,
    CAN_NOT_SAVE_STATE,
    CANT_RECALCULATE_ON_PRUNED_STATE,
    FAILED_TO_INITIALIZE_SET_ID,
    BAD_ORDER_OF_DIGEST_ITEM,
    UNKNOWN_DIGEST_TYPE
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::grandpa, AuthorityManagerError)
