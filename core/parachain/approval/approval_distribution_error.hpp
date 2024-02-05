/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/outcome.hpp"

namespace kagome::parachain {
  enum class ApprovalDistributionError {
    NO_INSTANCE = 1,
    NO_CONTEXT = 2,
    NO_SESSION_INFO = 3,
    UNUSED_SLOT_TYPE = 4,
    ENTRY_IS_NOT_FOUND = 5,
    ALREADY_APPROVING = 6,
    VALIDATOR_INDEX_OUT_OF_BOUNDS = 7,
    CORE_INDEX_OUT_OF_BOUNDS = 8,
    IS_IN_BACKING_GROUP = 9,
    SAMPLE_OUT_OF_BOUNDS = 10,
    VRF_DELAY_CORE_INDEX_MISMATCH = 11,
    VRF_VERIFY_AND_GET_TRANCHE = 12,
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, ApprovalDistributionError);
