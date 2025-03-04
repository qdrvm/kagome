/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/inherent_data.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::primitives, InherentDataError, e) {
  using E = kagome::primitives::InherentDataError;
  switch (e) {
    case E::IDENTIFIER_ALREADY_EXISTS:
      return "This identifier already exists";
    case E::IDENTIFIER_DOES_NOT_EXIST:
      return "This identifier does not exist";
  }
  return "Unknown error";
}
