/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_ERROR_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::consensus::grandpa {

  enum class VotingRoundError {
    FIN_VALIDATION_FAILED = 1,
    LAST_ESTIMATE_BETTER_THAN_PREVOTE,
    NEW_STATE_EQUAL_TO_OLD,
    NO_ESTIMATE_FOR_PREVIOUS_ROUND
  };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::grandpa, VotingRoundError);

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_ERROR_HPP
