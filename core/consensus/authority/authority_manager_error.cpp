/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/authority/authority_manager_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::authority, AuthorityManagerError, e) {
  using E = kagome::authority::AuthorityManagerError;
  switch (e) {
    case E::UNKNOWN_ENGINE_ID:
      return "Unknown engine_id";
    case E::ORPHAN_BLOCK_OR_ALREADY_FINALIZED:
      return "Block is not descendant of last finalized block";
    case E::CAN_NOT_SAVE_STATE:
      return "Can not save state";
  }
  return "unknown error (invalid AuthorityManagerError)";
}
