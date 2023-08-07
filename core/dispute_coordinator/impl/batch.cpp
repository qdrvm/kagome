/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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

    SL_INFO(log_, "DEBUG Batch::add_votes");  // XXX

    bool duplicate = true;

    {
      auto &[statement, index] = valid_vote;
      if (valid_votes_.emplace(index, statement).second) {
        ++votes_batched_since_last_tick_;
        SL_INFO(log_,  // XXX
                "DEBUG Batch::add_votes candidate {} - new valid",
                candidate_hash);
        duplicate = false;
      } else {
        SL_INFO(log_,  // XXX
                "DEBUG Batch::add_votes candidate {} - duplicate valid",
                candidate_hash);
      }
    }

    {
      auto &[statement, index] = invalid_vote;
      if (invalid_votes_.emplace(index, statement).second) {
        ++votes_batched_since_last_tick_;
        SL_INFO(log_,  // XXX
                "DEBUG Batch::add_votes candidate {} - new invalid",
                candidate_hash);
        duplicate = false;
      } else {
        SL_INFO(log_,  // XXX
                "DEBUG Batch::add_votes candidate {} - duplicate invalid",
                candidate_hash);
      }
    }

    SL_INFO(log_,  // XXX
            "DEBUG Batch::add_votes candidate {} - "
            "votes_batched_since_last_tick_={}",
            candidate_hash,
            votes_batched_since_last_tick_);

    if (duplicate) {
      SL_INFO(log_,  // XXX
              "DEBUG Batch::add_votes candidate {} - both duplicated",
              candidate_hash);
      return std::move(cb);
    }

    requesters_.push_back(std::make_tuple(peer, std::move(cb)));
    return std::nullopt;
  }

  std::optional<PreparedImport> Batch::tick(Batch::TimePoint now) {
    if (votes_batched_since_last_tick_ >= kMinKeepBatchAliveVotes
        and now < best_before_) {
      SL_INFO(log_,  // XXX
              "DEBUG Batch::tick candidate {} - still good",
              candidate_hash);

      // Still good:
      next_tick_time_ = now + kBatchCollectingInterval;
      if (next_tick_time_ > best_before_) {
        next_tick_time_ = best_before_;
      }
      // Reset counter:
      votes_batched_since_last_tick_ = 0;

      SL_INFO(log_,  // XXX
              "DEBUG Batch::tick candidate {} - reset counter",
              candidate_hash);

      return std::nullopt;
    }

    SL_INFO(log_,  // XXX
            "DEBUG Batch::tick candidate {} - prepare import",
            candidate_hash);

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

    SL_INFO(
        log_,  // XXX
        "DEBUG Batch::tick candidate {} - prepared import with {} statements",
        candidate_hash,
        prepared_import.statements.size());

    return prepared_import;
  }

}  // namespace kagome::dispute
