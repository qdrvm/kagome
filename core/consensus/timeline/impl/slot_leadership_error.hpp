/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>

namespace kagome::consensus {

  enum class SlotLeadershipError {
    NON_VALIDATOR = 1,
    DISABLED_VALIDATOR,
    NO_SLOT_LEADER,
    BACKING_OFF,
  };

}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, SlotLeadershipError)
