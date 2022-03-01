/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_HPP

#include "consensus/grandpa/chain.hpp"

#include <functional>
#include <memory>

#include "consensus/grandpa/common.hpp"
#include "consensus/grandpa/movable_round_state.hpp"
#include "consensus/grandpa/structs.hpp"
#include "primitives/block_id.hpp"

namespace kagome::blockchain {
  class BlockHeaderRepository;
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::primitives {
  struct Justification;
}
namespace libp2p::peer {
  class PeerId;
}
namespace kagome::consensus::grandpa {
  struct JustificationObserver;
}

namespace kagome::consensus::grandpa {

  class Grandpa;

  /**
   * Necessary environment for a voter.
   * This encapsulates the database and networking layers of the chain.
   */
  struct Environment : public virtual Chain {
    using CompleteHandler =
        std::function<void(outcome::result<MovableRoundState>)>;

    ~Environment() override = default;

    /**
     * Sets back-link to Grandpa
     */
    virtual void setJustificationObserver(
        std::weak_ptr<JustificationObserver> justification_observer) = 0;

    /**
     * Make cath-up-request
     */
    virtual outcome::result<void> onCatchUpRequested(
        const libp2p::peer::PeerId &peer_id,
        MembershipCounter set_id,
        RoundNumber round_number) = 0;

    /**
     * Make catch-up-response
     */
    virtual outcome::result<void> onCatchUpRespond(
        const libp2p::peer::PeerId &peer_id,
        MembershipCounter set_id,
        RoundNumber round_number,
        std::vector<SignedPrevote> prevote_justification,
        std::vector<SignedPrecommit> precommit_justification,
        BlockInfo best_final_candidate) = 0;

    virtual void sendState(const libp2p::peer::PeerId &peer_id,
                           const MovableRoundState &state,
                           MembershipCounter voter_set_id) = 0;

    /**
     * Note that we've done a vote in the given round.
     * Triggered when current peer appears in round \param round with
     * \param set_id and \param vote is ready to be sent.
     */
    virtual outcome::result<void> onVoted(RoundNumber round,
                                          MembershipCounter set_id,
                                          const SignedMessage &vote) = 0;

    /**
     * Triggered when current peer appears in round \param round with \param
     * voter_ser_id intends to send committed \param vote justified by \param
     * justification
     */
    virtual outcome::result<void> onCommitted(
        RoundNumber round,
        MembershipCounter voter_ser_id,
        const BlockInfo &vote,
        const GrandpaJustification &justification) = 0;

    /**
     * Triggered when current peer should send neighbor message
     * @param round current round
     * @param set_id id of actual voter set
     * @param last_finalized last known finalized block
     */
    virtual outcome::result<void> onNeighborMessageSent(
        RoundNumber round,
        MembershipCounter set_id,
        BlockNumber last_finalized) = 0;

    /**
     * Validate provided {@param justification} for finalization {@param block}.
     * If it valid finalize {@param block} and save {@param justification} in
     * storage.
     * @param block is observed block info
     * @param justification justification of finalization of provided block
     * @return nothing or on error
     */
    virtual outcome::result<void> applyJustification(
        const BlockInfo &block_info,
        const primitives::Justification &justification) = 0;

    /**
     * Triggered when blovk \param block justified by \param justification
     * should be applied to the storage
     */
    virtual outcome::result<void> finalize(
        MembershipCounter id, const GrandpaJustification &justification) = 0;

    virtual outcome::result<GrandpaJustification> getJustification(
        const BlockHash &block_hash) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_HPP
