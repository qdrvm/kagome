/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/block_tree_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain, BlockTreeError, e) {
  using E = kagome::blockchain::BlockTreeError;
  switch (e) {
    case E::NO_PARENT:
      return "block, which should have been added, has no known parent";
    case E::BLOCK_EXISTS:
      return "block, which should have been inserted, already exists in the "
             "tree";
    case E::SOME_BLOCK_IN_CHAIN_NOT_FOUND:
      return "one of the blocks for getting the chain was not found in the "
             "local storage";
    case E::TARGET_IS_PAST_MAX:
      return "target block number is past the given maximum number";
    case E::BLOCK_ON_DEAD_END:
      return "block resides on a dead fork";
    case E::EXISTING_BLOCK_NOT_FOUND:
      return "block exists in chain but not found when following all leaves "
             "backwards";
    case E::NON_FINALIZED_BLOCK_NOT_FOUND:
      return "a non-finalized block is not found";
    case E::JUSTIFICATION_NOT_FOUND:
      return "the requested justification is not found in block storage";
    case E::HEADER_NOT_FOUND:
      return "the requested block header is not found in block storage";
    case E::BODY_NOT_FOUND:
      return "the requested block body is not found in block storage";
    case E::BLOCK_IS_NOT_LEAF:
      return "the target block is not a leaf";
    case E::BLOCK_NOT_EXISTS:
      return "target block doesn't exist";
  }
  return "unknown error";
}
