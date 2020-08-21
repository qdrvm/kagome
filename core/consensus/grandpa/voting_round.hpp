/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTING_ROUND_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTING_ROUND_HPP

#include "consensus/grandpa/round_observer.hpp"
#include "consensus/grandpa/round_state.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Handles execution of one grandpa round. For details @see VotingRoundImpl
   */
  struct VotingRound : public std::enable_shared_from_this<VotingRound> {
    virtual ~VotingRound() = default;

    virtual RoundNumber roundNumber() const = 0;
    virtual std::shared_ptr<const RoundState> state() const = 0;

    virtual void play() = 0;
    virtual void end() = 0;

    /**
     * During the primary propose we:
     * 1. Check if we are the primary for the current round. If not execution of
     * the method is finished
     * 2. We can send primary propose only if the estimate from last round state
     * is greater than finalized. If we cannot send propose, method is finished
     * 3. Primary propose is the last rounds estimate.
     * 4. After all steps above are done we broadcast propose
     * 5. We store what we have broadcasted in primary_vote_ field
     */
    virtual void doProposal() = 0;

    /**
     * 1. Waits until start_time_ + duration * 2 or round is completable
     * 2. Constructs prevote (\see constructPrevote) and broadcasts it
     */
    virtual void doPrevote() = 0;

    /**
     * 1. Waits until start_time_ + duration * 4 or round is completable
     * 2. Constructs precommit (\see constructPrecommit) and broadcasts it
     */
    virtual void doPrecommit() = 0;

    // executes algorithm Attempt-To-Finalize-Round(r)
    virtual bool tryFinalize() = 0;

    virtual void onFinalize(const Fin &f) = 0;

    virtual void onPrimaryPropose(const SignedMessage &primary_propose) = 0;

    virtual void onPrevote(const SignedMessage &prevote) = 0;

    virtual void onPrecommit(const SignedMessage &precommit) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTING_ROUND_HPP
