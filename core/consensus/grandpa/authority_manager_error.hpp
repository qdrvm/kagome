/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/outcome.hpp"

namespace kagome::consensus::grandpa {
  enum class AuthorityManagerError {
    NOT_FOUND = 1,
    PREVIOUS_NOT_FOUND,
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::grandpa, AuthorityManagerError)
