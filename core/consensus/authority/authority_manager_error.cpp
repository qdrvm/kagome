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
  }
  return "unknown error (invalid AuthorityManagerError)";
}
