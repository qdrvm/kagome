/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/babe_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::babe, BabeError, e) {
  using E = kagome::consensus::babe::BabeError;
  switch (e) {
    case E::MISSING_PROOF:
      return "required VRF proof is missing";
    case E::BAD_ORDER_OF_DIGEST_ITEM:
      return "bad order of digest item; PreRuntime must be first";
    case E::UNKNOWN_DIGEST_TYPE:
      return "unknown type of digest";
  }
  return "unknown error";
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::babe, BlockAdditionError, e) {
  using E = kagome::consensus::babe::BlockAdditionError;
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
