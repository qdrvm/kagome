/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_VOTINGROUND
#define KAGOME_CONSENSUS_GRANDPA_VOTINGROUND

#include "consensus/grandpa/movable_round_state.hpp"
#include "consensus/grandpa/round_observer.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Handles execution of one grandpa round. For details @see VotingRoundImpl
   */
  struct VotingRound : public std::enable_shared_from_this<VotingRound> {
    virtual ~VotingRound() = default;

    // Getters

    virtual RoundNumber roundNumber() const = 0;
    virtual MembershipCounter voterSetId() const = 0;

    /**
     * Round is completable when we have block (stored in
     * current_state_.finalized) for which we have supermajority on both
     * prevotes and precommits
     */
    virtual bool completable() const = 0;

    virtual bool finalizable() const = 0;

    /// Block finalized in previous round (when current one was created)
    virtual BlockInfo lastFinalizedBlock() const = 0;

    /**
     * Best block from descendants of previous round best-final-candidate
     * @see spec: Best-PreVote-Candidate
     */
    virtual BlockInfo bestPrevoteCandidate() = 0;

    /**
     * Block what has prevote supermajority.
     * @see spec: Best-PreVote-Candidate
     * @see spec: Ghost-Function
     */
    virtual BlockInfo bestPrecommitCandidate() = 0;

    /**
     * Block what has precommit supermajority.
     * Should be descendant or equal of Best-PreVote-Candidate
     * @see spec: Best-Final-Candidate
     * @see spec: Ghost-Function
     */
    virtual BlockInfo bestFinalCandidate() = 0;

    /// Block is finalized at the round
    virtual boost::optional<BlockInfo> finalizedBlock() const = 0;

    virtual MovableRoundState state() const = 0;

    // Control lifecycle

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

    /// Broadcast Fin message
    virtual void doFinalize() = 0;

    /// Make Cathc-Up-Response based on current round and send to requesting
    /// peer
    virtual void doCatchUpResponse(const libp2p::peer::PeerId &peer_id) = 0;

    // Handling incoming messages

    enum class Propagation : bool { NEEDLESS = false, REQUESTED = true };

    /**
     * Invoked when we received a primary propose for this round
     */
    virtual void onProposal(const SignedMessage &primary_propose,
                            Propagation propagation) = 0;

    /**
     * Triggered when we receive {@param prevote} for current round.
     * Prevote will be propogated if {@param propagate} is true
     * @returns true if inner state has changed
     */
    virtual bool onPrevote(const SignedMessage &prevote,
                           Propagation propagation) = 0;

    /**
     * Triggered when we receive {@param precommit} for current round.
     * Precommit will be propogated if {@param propagate} is true
     * @returns true if inner state has changed
     */
    virtual bool onPrecommit(const SignedMessage &precommit,
                             Propagation propagation) = 0;

    /**
     * Updates inner state if {@param prevote} or {@param precommit} was changed
     * since last call
     */
    virtual void update(bool isPrevotesChanged, bool isPrecommitsChanged) = 0;

    // Auxiliary methods

    virtual outcome::result<void> applyJustification(
        const BlockInfo &block_info,
        const GrandpaJustification &justification) = 0;

    /// executes algorithm Attempt-To-Finalize-Round
    virtual void attemptToFinalizeRound() = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_VOTINGROUND
