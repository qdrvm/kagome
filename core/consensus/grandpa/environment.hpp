/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_HPP

#include "consensus/grandpa/chain.hpp"
#include "consensus/grandpa/common.hpp"
#include "consensus/grandpa/completed_round.hpp"
#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Necessary environment for a voter.
   * This encapsulates the database and networking layers of the chain.
   */
  struct Environment : public Chain {
    using CompleteHandler =
        std::function<void(outcome::result<CompletedRound>)>;

    ~Environment() override = default;

    // TODO(kamilsa): PRE-335 move timer to environment
    /** Return a timer that will be used to delay the broadcast of a commit
     * message. This delay should not be static to minimize the amount of
     * commit messages that are sent (e.g. random value in [0, 1] seconds).
     * virtual Timer roundCommitTimer() = 0;
     */

    /**
     * Note that we've done a primary proposal in the given round.
     * Triggered when current peer appears in round \param round with
     * \param set_id and \param propose is ready to be gossiped.
     */
    virtual outcome::result<void> onProposed(RoundNumber round,
                                             MembershipCounter set_id,
                                             const SignedMessage &propose) = 0;

    /**
     * Triggered when current peer appears in round \param round with
     * \param set_id and \param prevote is ready to be gossiped.
     */
    virtual outcome::result<void> onPrevoted(RoundNumber round,
                                             MembershipCounter set_id,
                                             const SignedMessage &prevote) = 0;

    /**
     * Triggered when current peer appears in round \param round with
     * \param set_id and \param precommit is ready to be gossiped.
     */
    virtual outcome::result<void> onPrecommitted(
        RoundNumber round,
        MembershipCounter set_id,
        const SignedMessage &precommit) = 0;

    /**
     * Triggered when current peer appears in round \param round intends to
     * gossip committed \param vote justified by \param justification
     */
    virtual outcome::result<void> onCommitted(
        RoundNumber round,
        const BlockInfo &vote,
        const GrandpaJustification &justification) = 0;

    /**
     * Provides a handler for completed round
     */
    virtual void doOnCompleted(const CompleteHandler &) = 0;

    /**
     * Triggered when round \param round is completed
     */
    virtual void onCompleted(outcome::result<CompletedRound> round) = 0;

    /**
     * Triggered when blovk \param block justified by \param justification
     * should be applied to the storage
     */
    virtual outcome::result<void> finalize(
        const primitives::BlockHash &block,
        const GrandpaJustification &justification) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_HPP
