/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/batch.hpp"

namespace kagome::dispute {

  Batch::Batch(CandidateHash candidate_hash,
               CandidateReceipt candidate_receipt,
               TimePoint now)
      : candidate_hash(std::move(candidate_hash)),
        candidate_receipt(std::move(candidate_receipt)),
        best_before_(now + kMaxBatchLifetime),
        log_(log::createLogger(fmt::format("Batch:{}", candidate_hash),
                               "dispute")) {}

  std::optional<CbOutcome<void>> Batch::add_votes(
      Indexed<SignedDisputeStatement> valid_vote,
      Indexed<SignedDisputeStatement> invalid_vote,
      const libp2p::peer::PeerId &peer,
      CbOutcome<void> &&cb) {
    BOOST_ASSERT(valid_vote.payload.candidate_hash == candidate_hash);
    BOOST_ASSERT(valid_vote.payload.candidate_hash
                 == invalid_vote.payload.candidate_hash);

    bool duplicate = true;

    {
      auto &[statement, index] = valid_vote;
      if (valid_votes_.emplace(index, statement).second) {
        ++votes_batched_since_last_tick_;
        duplicate = false;
      }
    }

    {
      auto &[statement, index] = invalid_vote;
      if (invalid_votes_.emplace(index, statement).second) {
        ++votes_batched_since_last_tick_;
        duplicate = false;
      }
    }

    if (duplicate) {
      return std::move(cb);
    }

    requesters_.push_back(std::make_tuple(peer, std::move(cb)));
    return std::nullopt;
  }

  std::optional<PreparedImport> Batch::tick(Batch::TimePoint now) {
    if (votes_batched_since_last_tick_ >= kMinKeepBatchAliveVotes
        and now < best_before_) {
      // Still good:
      next_tick_time_ = now + kBatchCollectingInterval;
      if (next_tick_time_ > best_before_) {
        next_tick_time_ = best_before_;
      }
      // Reset counter:
      votes_batched_since_last_tick_ = 0;
      return std::nullopt;
    }

    PreparedImport prepared_import{
        .candidate_receipt = candidate_receipt,
        .requesters = requesters_,
    };

    for (auto &votes : {valid_votes_, invalid_votes_}) {
      for (auto &vote : votes) {
        prepared_import.statements.emplace_back(
            Indexed<SignedDisputeStatement>{vote.second, vote.first});
      }
    }

    return prepared_import;
  }

}  // namespace kagome::dispute
