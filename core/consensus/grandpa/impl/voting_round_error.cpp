/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/voting_round_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::grandpa, VotingRoundError, e) {
  using E = kagome::consensus::grandpa::VotingRoundError;
  switch (e) {
    case E::FIN_VALIDATION_FAILED:
      return "Validation of finalization message failed";
    case E::SHOULD_NOT_PRECOMMIT:
      return "Should not precommit";
    case E::NEW_STATE_EQUAL_TO_OLD:
      return "New state is equal to the new one";
  }
}
