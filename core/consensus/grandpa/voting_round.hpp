/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTING_ROUND_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTING_ROUND_HPP

#include "consensus/grandpa/round_observer.hpp"
#include "consensus/grandpa/round_state.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Handles execution of one grandpa round. For details @see VotingRoundImpl
   */
  struct VotingRound {
    virtual ~VotingRound() = default;

    virtual void onFinalize(const Fin &f) = 0;

    virtual void onPrimaryPropose(
        const SignedPrimaryPropose &primary_propose) = 0;

    virtual void onPrevote(const SignedPrevote &prevote) = 0;

    virtual void onPrecommit(const SignedPrecommit &precommit) = 0;

    virtual void primaryPropose(const RoundState &last_round_state) = 0;

    virtual void prevote(const RoundState &last_round_state) = 0;

    virtual void precommit(const RoundState &last_round_state) = 0;

    // executes algorithm 4.9 Attempt-To-Finalize-Round(r)
    virtual bool tryFinalize() = 0;

    virtual RoundNumber roundNumber() const = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTING_ROUND_HPP
