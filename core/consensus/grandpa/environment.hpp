/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/chain.hpp"
#include "consensus/grandpa/justification_observer.hpp"

namespace kagome::primitives {
  struct Justification;
}

namespace libp2p::peer {
  class PeerId;
}

namespace kagome::consensus::grandpa {
  class Grandpa;
  struct MovableRoundState;
}  // namespace kagome::consensus::grandpa

namespace kagome::consensus::grandpa {

  /**
   * Necessary environment for a voter.
   * This encapsulates the database and networking layers of the chain.
   */
  class Environment : public virtual Chain {
   public:
    using ApplyJustificationCb = JustificationObserver::ApplyJustificationCb;
    ~Environment() override = default;

    /**
     * Make catch-up-request
     */
    virtual void onCatchUpRequested(const libp2p::peer::PeerId &peer_id,
                                    VoterSetId set_id,
                                    RoundNumber round_number) = 0;

    /**
     * Make catch-up-response
     */
    virtual void onCatchUpRespond(
        const libp2p::peer::PeerId &peer_id,
        VoterSetId set_id,
        RoundNumber round_number,
        std::vector<SignedPrevote> prevote_justification,
        std::vector<SignedPrecommit> precommit_justification,
        BlockInfo best_final_candidate) = 0;

    /**
     * Propagate round state
     */
    virtual void sendState(const libp2p::peer::PeerId &peer_id,
                           const MovableRoundState &state,
                           VoterSetId voter_set_id) = 0;

    /**
     * Note that we've done a vote in the given round.
     * Triggered when current peer appears in provided round with
     * provided set_id and given vote is ready to be sent.
     */
    virtual void onVoted(RoundNumber round,
                         VoterSetId set_id,
                         const SignedMessage &vote) = 0;

    /**
     * Triggered when current peer appears in given round and has given
     * voter_ser_id intends to send committed vote justified by provided
     * justification
     */
    virtual void onCommitted(RoundNumber round,
                             VoterSetId voter_ser_id,
                             const BlockInfo &vote,
                             const GrandpaJustification &justification) = 0;

    /**
     * Triggered when current peer should send neighbor message
     * @param round current round
     * @param set_id id of actual voter set
     * @param last_finalized last known finalized block
     */
    virtual void onNeighborMessageSent(RoundNumber round,
                                       VoterSetId set_id,
                                       BlockNumber last_finalized) = 0;

    /**
     * Validate provided {@param justification} for finalization {@param block}.
     * If it valid finalize {@param block} and save {@param justification} in
     * storage.
     * @param block is observed block info
     * @param justification justification of finalization of provided block
     * @return nothing or on error
     */
    virtual void applyJustification(
        const BlockInfo &block_info,
        const primitives::Justification &justification,
        ApplyJustificationCb &&cb) = 0;

    /**
     * Triggered when blovk \param block justified by \param justification
     * should be applied to the storage
     */
    virtual outcome::result<void> finalize(
        VoterSetId id, const GrandpaJustification &justification) = 0;

    /**
     * Returns justification for given block
     * @param block_hash hash of the block that we are trying to get
     * justification for
     * @return GrandpaJustification containing signed precommits for given block
     * if there is such justification. Error otherwise
     */
    virtual outcome::result<GrandpaJustification> getJustification(
        const BlockHash &block_hash) = 0;
  };

}  // namespace kagome::consensus::grandpa
