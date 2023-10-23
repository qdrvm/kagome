/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/tagged.hpp"
#include "consensus/grandpa/movable_round_state.hpp"
#include "consensus/grandpa/round_observer.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Handles execution of one grandpa round. For details @see VotingRoundImpl
   */
  class VotingRound {
   public:
    virtual ~VotingRound() = default;

    // Getters

    virtual RoundNumber roundNumber() const = 0;

    virtual VoterSetId voterSetId() const = 0;

    virtual bool completable() const = 0;

    virtual BlockInfo lastFinalizedBlock() const = 0;

    virtual BlockInfo bestFinalCandidate() = 0;

    virtual const std::optional<BlockInfo> &finalizedBlock() const = 0;

    virtual MovableRoundState state() const = 0;

    // Control lifecycle

    virtual bool hasKeypair() const = 0;
    virtual void play() = 0;
    virtual void end() = 0;

    // Doing some activity

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

    /// Calculate prevote and broadcast signed prevote message
    virtual void doPrevote() = 0;

    /// Calculate precommit and broadcast signed precommit message
    virtual void doPrecommit() = 0;

    /// Collect and save justifications finalizing this round
    virtual void doFinalize() = 0;

    /// Broadcast commit message
    virtual void doCommit() = 0;

    /// Make Catch-Up-Response based on current round and send to requesting
    /// peer
    virtual void doCatchUpResponse(const libp2p::peer::PeerId &peer_id) = 0;

    // Handling incoming messages

    enum class Propagation : bool { NEEDLESS = false, REQUESTED = true };

    virtual void onProposal(std::optional<GrandpaContext> &grandpa_context,
                            const SignedMessage &primary_propose,
                            Propagation propagation) = 0;

    virtual bool onPrevote(std::optional<GrandpaContext> &grandpa_context,
                           const SignedMessage &prevote,
                           Propagation propagation) = 0;

    virtual bool onPrecommit(std::optional<GrandpaContext> &grandpa_context,
                             const SignedMessage &precommit,
                             Propagation propagation) = 0;

    using IsPreviousRoundChanged = Tagged<bool, struct IsPreviousRoundChanged>;
    using IsPrevotesChanged = Tagged<bool, struct IsPrevotesChanged>;
    using IsPrecommitsChanged = Tagged<bool, struct IsPrecommitsChanged>;

    /**
     * Updates inner state if something (see params) was changed since last call
     * @param is_previous_round_changed is true if previous round is changed
     * @param is_prevotes_changed is true if new prevote was accepted
     * @param is_precommits_changed is true if new precommits was accepted
     * @return true if finalized block was changed during update
     */
    virtual void update(IsPreviousRoundChanged is_previous_round_changed,
                        IsPrevotesChanged is_prevotes_changed,
                        IsPrecommitsChanged is_precommits_changed) = 0;

    // Auxiliary methods

    /**
     * @returns previous known round for current
     */
    virtual std::shared_ptr<VotingRound> getPreviousRound() const = 0;

    /**
     * Removes previous round to limit chain of rounds
     */
    virtual void forgetPreviousRound() = 0;

    virtual outcome::result<void> applyJustification(
        const GrandpaJustification &justification) = 0;

    /// executes algorithm Attempt-To-Finalize-Round
    virtual void attemptToFinalizeRound() = 0;
  };

}  // namespace kagome::consensus::grandpa
