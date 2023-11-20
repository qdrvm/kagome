/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/block_production_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, BlockProductionError, e) {
  using E = kagome::consensus::BlockProductionError;
  switch (e) {
    case E::CAN_NOT_PREPARE_BLOCK:
      return "can not prepare block";
    case E::CAN_NOT_SEAL_BLOCK:
      return "can not seal block";
    case E::WAS_NOT_BUILD_ON_TIME:
      return "block was not build on time";
    case E::CAN_NOT_SAVE_BLOCK:
      return "can not save block";
  }
  return "unknown error (kagome::consensus::BlockProductionError)";
}
