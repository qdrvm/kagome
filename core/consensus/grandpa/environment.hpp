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

  /// Necessary environment for a voter.
  ///
  /// This encapsulates the database and networking layers of the chain.
  struct Environment : public Chain {
    ~Environment() override = default;

    // TODO(kamilsa): PRE-335 move timer to environment
    /// Return a timer that will be used to delay the broadcast of a commit
    /// message. This delay should not be static to minimize the amount of
    /// commit messages that are sent (e.g. random value in [0, 1] seconds).
    // virtual Timer roundCommitTimer() = 0;

    /// Note that we've done a primary proposal in the given round.
    /**
     * Triggered when current peer appeared in round \param round and having
     * \param set_id has \param propose ready to be gossiped.
     */
    virtual outcome::result<void> proposed(
        RoundNumber round,
        MembershipCounter set_id,
        const SignedPrimaryPropose &propose) = 0;

    /**
     * Triggered when current peer appeared in round \param round and having
     * \param set_id has \param prevote ready to be gossiped.
     */
    virtual outcome::result<void> prevoted(RoundNumber round,
                                           MembershipCounter set_id,
                                           const SignedPrevote &prevote) = 0;

    /**
     * Triggered when current peer appeared in round \param round and having
     * \param set_id has \param precommit ready to be gossiped.
     */
    virtual outcome::result<void> precommitted(
        RoundNumber round,
        MembershipCounter set_id,
        const SignedPrecommit &precommit) = 0;

    /**
     * Triggered when current peer appeared in round \param round intends to
     * gossip committed \param vote justified by \param justification
     */
    virtual outcome::result<void> commit(
        RoundNumber round,
        const BlockInfo &vote,
        const GrandpaJustification &justification) = 0;

    virtual void onCompleted(std::function<void(const CompletedRound &)>) = 0;

    /**
     * Triggered when round \param round is completed
     */
    virtual void completed(CompletedRound round) = 0;

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
