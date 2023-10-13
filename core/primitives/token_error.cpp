/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/token_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::primitives, TokenError, e) {
  using E = kagome::primitives::TokenError;
  switch (e) {
    case E::NoFunds:
      return "Funds are unavailable";
    case E::WouldDie:
      return "Account that must exist would die";
    case E::BelowMinimum:
      return "Account cannot exist with the funds that would be given";
    case E::CannotCreate:
      return "Account cannot be created";
    case E::UnknownAsset:
      return "The asset in question is unknown";
    case E::Frozen:
      return "Funds exist but are frozen";
    case E::Unsupported:
      return "Operation is not supported by the asset";
  }
  return "Unknown TokenError";
}
