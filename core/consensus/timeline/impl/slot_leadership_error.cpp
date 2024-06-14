/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/slot_leadership_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, SlotLeadershipError, e) {
  using E = kagome::consensus::SlotLeadershipError;
  switch (e) {
    case E::NON_VALIDATOR:
      return "node is not validator in current epoch";
    case E::DISABLED_VALIDATOR:
      return "node is disabled validator till end of epoch";
    case E::NO_SLOT_LEADER:
      return "node is not slot leader in current slot";
    case E::BACKING_OFF:
      return "backing off claiming new slot for block authorship";
  }
  return "unknown error (kagome::consensus::SlotLeadershipError)";
}
