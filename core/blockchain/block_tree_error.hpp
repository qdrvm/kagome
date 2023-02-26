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
    NO_PARENT = 1,
    BLOCK_EXISTS,
    // target block number is past the given maximum number
    TARGET_IS_PAST_MAX,
    // block resides on a dead fork
    BLOCK_ON_DEAD_END,
    // block exists in chain but not found when following all
    // leaves backwards
    EXISTING_BLOCK_NOT_FOUND,
    // non-finalized block is not found
    NON_FINALIZED_BLOCK_NOT_FOUND,
    // justification is not found in block storage
    JUSTIFICATION_NOT_FOUND,
    // block body is not found in block storage
    BODY_NOT_FOUND,
    // block header is not found in block storage
    HEADER_NOT_FOUND,
    // some block in the requested chain is missing
    SOME_BLOCK_IN_CHAIN_NOT_FOUND,
    // block is not a leaf
    BLOCK_IS_NOT_LEAF,
    BLOCK_NOT_EXISTS
  };
}  // namespace kagome::blockchain

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, BlockTreeError)

#endif  // KAGOME_BLOCK_TREE_ERROR_HPP
