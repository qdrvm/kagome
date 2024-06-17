/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/dispute_request_observer.hpp"

#include "dispute_coordinator/participation/types.hpp"
#include "dispute_coordinator/types.hpp"

/**
* Disputes contains 2 main classes: dispute coordinator and participation.
The main purpose of coordinator is to restore previous state once on
startup(because of possible previous disputes on previous launch session), to
initialize disputes based on approval result and to handle and collect
statements from the whole validators set. Blocks with either active disputes or
invalid are blocked for finalization, while valid disputes are allowed to be
finalized. The main goal of participation class is to launch our validation
process with predefined limits. It initializez recovery process followed by
exhaustive validation and the result statement. Which will be imported the same
way as statements from other nodes. So the disputes workflow is a loop which
enters on disputes initialization either disputes request or approval result and
works until enough votes collected to make a decision about block validity or
another fork will be finalized.
*/

namespace kagome::dispute {

  class DisputeCoordinator {
   public:
    using QueryCandidateVotes =
        std::vector<std::tuple<SessionIndex, CandidateHash>>;
    using OutputCandidateVotes =
        std::vector<std::tuple<SessionIndex, CandidateHash, CandidateVotes>>;
    using OutputDisputes =
        std::vector<std::tuple<SessionIndex, CandidateHash, DisputeStatus>>;

    virtual ~DisputeCoordinator() = default;

    /// Fetch a list of all recent disputes the coordinator is aware of.
    /// These are disputes which have occurred any time in recent sessions,
    /// and which may have already concluded.
    virtual void getRecentDisputes(CbOutcome<OutputDisputes> &&cb) = 0;

    virtual void onParticipation(const ParticipationStatement &message) = 0;

    /// Fetch a list of all active disputes that the coordinator is aware of.
    /// These disputes are either not yet concluded or recently concluded.
    virtual void getActiveDisputes(CbOutcome<OutputDisputes> &&cb) = 0;

    /// Get candidate votes for a candidate (QueryCandidateVotes)
    virtual void queryCandidateVotes(const QueryCandidateVotes &msg,
                                     CbOutcome<OutputCandidateVotes> &&cb) = 0;

    /// Sign and issue local dispute votes. A value of `true` indicates
    /// validity, and `false` invalidity.
    virtual void issueLocalStatement(SessionIndex session,
                                     CandidateHash candidate_hash,
                                     CandidateReceipt candidate_receipt,
                                     bool valid) = 0;

    /// Determine the highest undisputed block within the given chain, based on
    /// where candidates were included. If even the base block should not be
    /// finalized due to a dispute, then `None` should be returned on the
    /// channel.
    ///
    /// The block descriptions begin counting upwards from the block after the
    /// given `base_number`. The `base_number` is typically the number of the
    /// last finalized block but may be slightly higher. This block is
    /// inevitably going to be finalized so it is not accounted for by
    /// thisfunction.
    ///
    /// @param base - The lowest possible block to vote on
    /// @param block_descriptions - Descriptions of all the blocks counting
    /// upwards from the block after the base number
    /// @param cb - Callback for result
    /// @note The block to vote on, might be base in case there is no better.
    virtual void determineUndisputedChain(
        primitives::BlockInfo base,
        std::vector<BlockDescription> block_descriptions,
        CbOutcome<primitives::BlockInfo> &&cb) = 0;

    virtual void getDisputeForInherentData(
        const primitives::BlockInfo &relay_parent,
        std::function<void(MultiDisputeStatementSet)> &&cb) = 0;
  };

}  // namespace kagome::dispute
