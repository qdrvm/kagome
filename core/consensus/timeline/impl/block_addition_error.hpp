/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>

namespace kagome::consensus {

  enum class BlockAdditionError {
    ORPHAN_BLOCK = 1,
    BLOCK_MISSING_HEADER,
    PARENT_NOT_FOUND,
    NO_INSTANCE
  };
}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, BlockAdditionError)
