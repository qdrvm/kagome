/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_CANDIDATEVOTESTATE
#define KAGOME_DISPUTE_CANDIDATEVOTESTATE

#include "dispute_coordinator/types.hpp"

namespace kagome::dispute {

  /// Complete state of votes for a candidate.
  ///
  /// All votes + information whether a dispute is ongoing, confirmed,
  /// concluded, whether we already voted, ...
  class CandidateVoteState final {
   public:
    static CandidateVoteState create(CandidateVotes votes,
                                     CandidateEnvironment &env,
                                     Timestamp now);

    /// Votes already existing for the candidate + receipt.
    CandidateVotes votes;

    /// Information about own votes:
    OwnVoteState own_vote = CannotVote{};

    /// Current dispute status, if there is any.
    std::optional<DisputeStatus> dispute_status;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_CANDIDATEVOTESTATE
