/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>

namespace kagome::consensus {

  enum class TimelineError {
    SLOT_BEFORE_GENESIS = 1,
  };

}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, TimelineError)
