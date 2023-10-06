/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/outcome.hpp"

namespace kagome::blockchain {

  enum class BlockStorageError {
    BLOCK_EXISTS = 1,
    HEADER_NOT_FOUND,
    GENESIS_BLOCK_ALREADY_EXISTS,
    GENESIS_BLOCK_NOT_FOUND,
    FINALIZED_BLOCK_NOT_FOUND,
    BLOCK_TREE_LEAVES_NOT_FOUND
  };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, BlockStorageError);
