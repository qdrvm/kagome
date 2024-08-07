/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "dispute_coordinator/types.hpp"
#include "network/types/collator_messages.hpp"
#include "parachain/types.hpp"

namespace kagome::dispute {

  using network::CandidateHash;
  using network::SessionIndex;
  using network::ValidatorIndex;

  class Storage {
   public:
    virtual ~Storage() = default;

    /// Load the candidate votes for the specific session-candidate pair, if any
    virtual outcome::result<std::optional<CandidateVotes>> load_candidate_votes(
        SessionIndex session, const CandidateHash &candidate_hash) = 0;

    virtual void write_candidate_votes(SessionIndex session,
                                       const CandidateHash &candidate_hash,
                                       const CandidateVotes &votes) = 0;

    /// Load the recent disputes, if any.
    virtual outcome::result<std::optional<RecentDisputes>>
    load_recent_disputes() = 0;

    /// Prepare a write to the recent disputes stored in the DB.
    ///
    /// Later calls to this function will override earlier ones.
    virtual void write_recent_disputes(RecentDisputes recent_disputes) = 0;
  };

}  // namespace kagome::dispute
