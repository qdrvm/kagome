/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_AUTHORITIES_MANAGER_ERROR
#define KAGOME_CONSENSUS_AUTHORITIES_MANAGER_ERROR

#include <outcome/outcome.hpp>

namespace kagome::authority {
  enum class AuthorityManagerError {
    UNKNOWN_ENGINE_ID = 1,
    ORPHAN_BLOCK_OR_ALREADY_FINALIZED,
    CAN_NOT_SAVE_STATE,
    CANT_RECALCULATE_ON_PRUNED_STATE,
    FAILED_TO_INITIALIZE_SET_ID,
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::authority, AuthorityManagerError)

#endif  // KAGOME_CONSENSUS_AUTHORITIES_MANAGER_ERROR
