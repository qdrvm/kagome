/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authority_manager_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::grandpa,
                            AuthorityManagerError,
                            e) {
  using E = decltype(e);
  switch (e) {
    case E::NOT_FOUND:
      return "grandpa authorities not found";
    case E::PREVIOUS_NOT_FOUND:
      return "previous grandpa authorities not found";
  }
  abort();
}
