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
    case E::WRONG_FINALISATION_ORDER:
      return "Finalisation of block which isn't descendant of last finalised";
    case E::ORPHAN_BLOCK_OR_ALREADY_FINALISED:
      return "Block it not descendant of last finalised block";
  }
  return "unknown error (invalid AuthorityManagerError)";
}
