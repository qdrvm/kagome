/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/outcome.hpp"

namespace kagome::authorship {

  enum class BlockBuilderError {
    EXTRINSIC_APPLICATION_FAILED = 1,
    EXHAUSTS_RESOURCES,
    BAD_MANDATORY,
  };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::authorship, BlockBuilderError)
