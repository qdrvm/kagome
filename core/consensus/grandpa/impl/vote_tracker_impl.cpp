/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/vote_tracker_impl.hpp"

#include "common/visitor.hpp"
#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  VoteTracker::PushResult VoteTrackerImpl::push(const SignedMessage &vote,
                                                size_t weight) {
    auto vote_it = messages_.find(vote.id);
    if (vote_it == messages_.end()) {
      messages_[vote.id] = vote;
      total_weight_ += weight;
      return PushResult::SUCCESS;
    }

    auto &known_vote_variant = vote_it->second;

    return visit_in_place(
        known_vote_variant,
        [&](const SignedMessage &known_vote) {
          // If it is known vote, it means duplicate
          if (vote == known_vote) {
            return PushResult::DUPLICATED;
          }

          // Otherwise, it's another vote of known voter, make it equivocation
          messages_[vote.id] = EquivocatorySignedMessage(known_vote, vote);
          return PushResult::EQUIVOCATED;
        },
        [](const EquivocatorySignedMessage &) {
          // This is vote of known equivocator
          return PushResult::EQUIVOCATED;
        });
  }

  void VoteTrackerImpl::unpush(const SignedMessage &vote, size_t weight) {
    auto vote_it = messages_.find(vote.id);
    if (vote_it == messages_.end()) {
      return;
    }
    auto &existing_vote = vote_it->second;
    visit_in_place(
        existing_vote,
        [&](const SignedMessage &voting_message) {
          if (voting_message == vote) {
            messages_.erase(vote_it);
            total_weight_ -= weight;
          }
        },
        [](const auto &...) {});
  }

  std::vector<VoteVariant> VoteTrackerImpl::getMessages() const {
    std::vector<VoteVariant> votes;
    // the actual number may be bigger, but it's a good guess
    votes.reserve(messages_.size());
    for (const auto &[key, value] : messages_) {
      votes.push_back(value);
    }
    return votes;
  }

  size_t VoteTrackerImpl::getTotalWeight() const {
    return total_weight_;
  }

}  // namespace kagome::consensus::grandpa
