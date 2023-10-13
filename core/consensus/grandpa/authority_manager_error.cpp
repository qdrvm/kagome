/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authority_manager_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::grandpa,
                            AuthorityManagerError,
                            e) {
  using E = kagome::consensus::grandpa::AuthorityManagerError;
  switch (e) {
    case E::UNKNOWN_ENGINE_ID:
      return "Unknown engine_id";
    case E::ORPHAN_BLOCK_OR_ALREADY_FINALIZED:
      return "Block is not descendant of last finalized block";
    case E::CAN_NOT_SAVE_STATE:
      return "Can not save state";
    case E::CANT_RECALCULATE_ON_PRUNED_STATE:
      return "Can't recalculate authority set ids on a pruned database";
    case E::FAILED_TO_INITIALIZE_SET_ID:
      return "Failed to initialize the current authority set id on startup";
    case E::BAD_ORDER_OF_DIGEST_ITEM:
      return "bad order of digest item; PreRuntime must be first";
    case E::UNKNOWN_DIGEST_TYPE:
      return "unknown type of digest";
  }
  return "unknown error (invalid AuthorityManagerError)";
}
