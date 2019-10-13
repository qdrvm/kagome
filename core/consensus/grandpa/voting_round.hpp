/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTING_ROUND_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTING_ROUND_HPP

#include "consensus/grandpa/round_observer.hpp"
#include "consensus/grandpa/round_state.hpp"

namespace kagome::consensus::grandpa {

  struct VotingRound {
    virtual ~VotingRound() = default;

    void onFin(const Fin &f);

    virtual void onPrimaryPropose(
        const SignedPrimaryPropose &primary_propose) = 0;

    virtual void onPrevote(const SignedPrevote &prevote) = 0;

    virtual void onPrecommit(const SignedPrecommit &precommit) = 0;

    virtual void primaryPropose(const RoundState &last_round_state) = 0;

    virtual void prevote(const RoundState &last_round_state) = 0;

    virtual void precommit(const RoundState &last_round_state) = 0;

    // executes algorithm 4.9 Attempt-To-Finalize-Round(r)
    virtual void tryFinalize() = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTING_ROUND_HPP
