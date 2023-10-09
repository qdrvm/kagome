/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/candidate_vote_state.hpp"
#include <set>

namespace kagome::dispute {
  CandidateVoteState CandidateVoteState::create(CandidateVotes votes,
                                                CandidateEnvironment &env,
                                                Timestamp now) {
    CandidateVoteState res{.votes = std::move(votes),
                           .own_vote = CannotVote{},
                           .dispute_status = std::nullopt};

    auto controlled_indices = env.controlled_indices;
    if (not controlled_indices.empty()) {
      Voted voted;

      for (auto index : controlled_indices) {
        auto it = res.votes.valid.find(index);
        if (it != res.votes.valid.end()) {
          auto &[dispute_statement, validator_signature] = it->second;
          voted.emplace_back(index, dispute_statement, validator_signature);
        }
      }

      for (auto index : controlled_indices) {
        auto it = res.votes.invalid.find(index);
        if (it != res.votes.invalid.end()) {
          auto &[dispute_statement, validator_signature] = it->second;
          voted.emplace_back(index, dispute_statement, validator_signature);
        }
      }

      if (not voted.empty()) {
        res.own_vote = std::move(voted);
      }
    }

    // Check if isn't disputed
    // (We have a dispute, if we have votes on both sides)
    if (res.votes.invalid.empty() or res.votes.valid.empty()) {
      return res;
    }

    auto n_validators = env.session.validators.size();

    auto byzantine_threshold = (std::max<size_t>(n_validators, 1) - 1) / 3;
    auto supermajority_threshold = n_validators - byzantine_threshold;

    DisputeStatus status = Active{};

    // Check if concluded for
    if (res.votes.valid.size() >= supermajority_threshold) {
      res.dispute_status = ConcludedFor{now};
      return res;
    }

    // Check if concluded against
    if (res.votes.invalid.size() >= supermajority_threshold) {
      res.dispute_status = ConcludedAgainst{now};
      return res;
    }

    // Check if confirmed
    if (res.votes.valid.size() > byzantine_threshold
        or res.votes.invalid.size() > byzantine_threshold) {
      res.dispute_status = Confirmed{};
      return res;
    }
    std::set<ValidatorIndex> v;
    for (auto &[i, _] : res.votes.valid) {
      v.emplace(i);
    }
    for (auto &[i, _] : res.votes.invalid) {
      v.emplace(i);
      if (v.size() > byzantine_threshold) {
        res.dispute_status = Confirmed{};
        return res;
      }
    }

    // Active otherwise
    res.dispute_status = Active{};
    return res;
  }

}  // namespace kagome::dispute
