/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>

namespace kagome::consensus {

  enum class BlockProductionError {
    CAN_NOT_PREPARE_BLOCK = 1,
    CAN_NOT_SEAL_BLOCK,
    WAS_NOT_BUILD_ON_TIME,
    CAN_NOT_SAVE_BLOCK,
  };

}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, BlockProductionError)
