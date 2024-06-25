/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "dispute_coordinator/types.hpp"

namespace kagome::dispute {

  /// Complete state of votes for a candidate.
  ///
  /// All votes + information whether a dispute is ongoing, confirmed,
  /// concluded, whether we already voted, ...
  class CandidateVoteState final {
   public:
    /**
     * Creates CandidateVoteState based on collected votes, environment and
     * taking into account disabled validators
     * @param votes already collected votes for dispute
     * @param env related data
     * @param disabled presorted list of disabled validator indices
     * @param now current timestamp
     * @return CandidateVoteState with correct inner data
     */
    static CandidateVoteState create(CandidateVotes votes,
                                     CandidateEnvironment &env,
                                     std::vector<ValidatorIndex> &disabled,
                                     Timestamp now);

    /// Votes already existing for the candidate + receipt.
    CandidateVotes votes;

    /// Information about own votes:
    OwnVoteState own_vote = CannotVote{};

    /// Current dispute status, if there is any.
    std::optional<DisputeStatus> dispute_status;
  };

}  // namespace kagome::dispute
