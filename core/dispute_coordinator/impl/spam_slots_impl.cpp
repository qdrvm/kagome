/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/spam_slots_impl.hpp"

namespace kagome::dispute {

  bool SpamSlotsImpl::add_unconfirmed(SessionIndex session,
                                      CandidateHash candidate,
                                      ValidatorIndex validator) {
    auto &spam_vote_count =
        slots_.emplace(std::tuple(session, validator), 0u).first->second;
    if (spam_vote_count >= kMaxSpamVotes) {
      return false;
    }

    auto &validators =
        unconfirmed_
            .emplace(std::tuple(session, candidate), std::set<ValidatorIndex>{})
            .first->second;

    if (validators.emplace(validator).second) {
      // We only increment spam slots once per candidate, as each validator has
      // to provide an opposing vote for sending out its own vote. Therefore,
      // receiving multiple votes for a single candidate is expected and should
      // not get punished here.
      ++spam_vote_count;
    }

    return true;
  }

  void SpamSlotsImpl::clear(SessionIndex session, CandidateHash candidate) {
    auto unconfirmed_it = unconfirmed_.find(std::tuple(session, candidate));
    if (unconfirmed_it != unconfirmed_.end()) {
      auto &validators = unconfirmed_it->second;
      for (auto validator : validators) {
        if (auto slots_it = slots_.find(std::tuple(session, validator));
            slots_it != slots_.end()) {
          auto &spam_vote_count = slots_it->second;
          if (--spam_vote_count == 0) {
            slots_.erase(slots_it);
          }
        }
      }
      unconfirmed_.erase(unconfirmed_it);
    }
  }

  void SpamSlotsImpl::prune_old(SessionIndex oldest_index) {
    for (auto it = unconfirmed_.begin(); it != unconfirmed_.end();) {
      auto cit = it++;
      if (std::get<0>(cit->first) < oldest_index) {
        unconfirmed_.erase(cit);
      }
    }
    for (auto it = slots_.begin(); it != slots_.end();) {
      auto cit = it++;
      if (std::get<0>(cit->first) < oldest_index) {
        slots_.erase(cit);
      }
    }
  }

}  // namespace kagome::dispute
