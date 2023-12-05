/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/block_addition_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, BlockAdditionError, e) {
  using E = kagome::consensus::BlockAdditionError;
  switch (e) {
    case E::ORPHAN_BLOCK:
      return "Attempt to append a block which is either already finalized or "
             "not a descendant of any known block";
    case E::BLOCK_MISSING_HEADER:
      return "Block without a header cannot be appended";
    case E::PARENT_NOT_FOUND:
      return "Parent not found";
    case E::NO_INSTANCE:
      return "No instance";
  }
  return "Unknown error";
}
