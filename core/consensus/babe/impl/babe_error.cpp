/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::babe, BabeError, e) {
  using E = kagome::consensus::babe::BabeError;
  switch (e) {
    case E::NO_VALIDATOR:
      return "node is not validator in current epoch";
    case E::NO_SLOT_LEADER:
      return "node is not slot leader in current slot";
    case E::BACKING_OFF:
      return "backing off claiming new slot for block authorship";
    case E::CAN_NOT_PREPARE_BLOCK:
      return "can not prepare block";
    case E::CAN_NOT_PROPOSE_BLOCK:
      return "can not propose block";
    case E::CAN_NOT_SEAL_BLOCK:
      return "can not seal block";
    case E::WAS_NOT_BUILD_ON_TIME:
      return "block was not build on time";
    case E::CAN_NOT_SAVE_BLOCK:
      return "can not save block";
    case E::MISSING_PROOF:
      return "required VRF proof is missing";
    case E::BAD_ORDER_OF_DIGEST_ITEM:
      return "bad order of digest item; PreRuntime must be first";
    case E::UNKNOWN_DIGEST_TYPE:
      return "unknown type of digest";
    case E::SLOT_BEFORE_GENESIS:
      return "slot before genesis";
  }
  return "unknown error";
}
