/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/validator/prospective_parachains/common.hpp"

namespace kagome::parachain::fragment {

  enum FragmentChainError : uint8_t {
    CANDIDATE_ALREADY_KNOWN = 1,
    INTRODUCE_BACKED_CANDIDATE,
    CYCLE,
    MULTIPLE_PATH,
    ZERO_LENGTH_CYCLE,
    RELAY_PARENT_NOT_IN_SCOPE,
    RELAY_PARENT_PRECEDES_CANDIDATE_PENDING_AVAILABILITY,
    FORK_WITH_CANDIDATE_PENDING_AVAILABILITY,
    FORK_CHOICE_RULE,
    PARENT_CANDIDATE_NOT_FOUND,
    COMPUTE_CONSTRAINTS,
    CHECK_AGAINST_CONSTRAINTS,
    RELAY_PARENT_MOVED_BACKWARDS,
  };

}  // namespace kagome::parachain::fragment

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain::fragment, FragmentChainError)
