/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/block_production_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, BlockProductionError, e) {
  using E = kagome::consensus::BlockProductionError;
  switch (e) {
    case E::NO_VALIDATOR:
      return "node is not validator in current epoch";
    case E::NO_SLOT_LEADER:
      return "node is not slot leader in current slot";
    case E::BACKING_OFF:
      return "backing off claiming new slot for block authorship";
  }
  return "unknown error (kagome::consensus::BlockProductionError)";
}
