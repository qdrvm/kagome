/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_TREE_ERROR_HPP
#define KAGOME_BLOCK_TREE_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::blockchain {
  /**
   * Errors of the block tree are here, so that other modules can use them, for
   * example, to compare a received error with those
   */
  enum class BlockTreeError {
    INVALID_DB = 1,
    NO_PARENT,
    BLOCK_EXISTS,
    HASH_FAILED,
    NO_SUCH_BLOCK,
    INCORRECT_ARGS,
    INTERNAL_ERROR
  };
}  // namespace kagome::blockchain

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, BlockTreeError)

#endif  // KAGOME_BLOCK_TREE_ERROR_HPP
