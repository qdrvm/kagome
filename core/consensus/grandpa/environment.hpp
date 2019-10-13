/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_HPP

#include "consensus/grandpa/message.hpp"
#include "consensus/grandpa/round.hpp"

namespace kagome::consensus::grandpa {

  /// Necessary environment for a voter.
  ///
  /// This encapsulates the database and networking layers of the chain.
  struct Environment {
    virtual ~Environment() = default;

    /// Produce data necessary to start a round of voting.
    virtual RoundData roundData(RoundNumber round) = 0;

    /// Return a timer that will be used to delay the broadcast of a commit
    /// message. This delay should not be static to minimize the amount of
    /// commit messages that are sent (e.g. random value in [0, 1] seconds).
    virtual Timer roundCommitTimer() = 0;

    /// Note that we've done a primary proposal in the given round.
    virtual outcome::result<void> proposed(RoundNumber round,
                                           PrimaryPropose propose) = 0;

    /// Note that we have prevoted in the given round.
    virtual outcome::result<void> prevoted(RoundNumber round,
                                           Prevote prevote) = 0;

    /// Note that we have precommitted in the given round.
    virtual outcome::result<void> precommitted(RoundNumber round,
                                               Precommit precommit) = 0;

    /// Note that a round was completed. This is called when a round has been
    /// voted in. Should return an error when something fatal occurs.
    virtual outcome::result<void> completed(RoundNumber round,
                                            RoundState state,
                                            BlockInfo base,
                                            HistoricalVotes &votes) = 0;

    /// Called when a block should be finalized.
    virtual outcome::result<void> finalizeBlock(RoundNumber round,
                                                BlockInfo block,
                                                Commit commit) = 0;

    virtual void prevoteEquivocation(RoundNumber round,
                                     PrevoteEquivocation eq) = 0;

    virtual void precommitEquivocation(RoundNumber round,
                                       PrecommitEquivocation eq) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_HPP
