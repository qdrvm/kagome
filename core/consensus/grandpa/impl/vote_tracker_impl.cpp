/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "common/visitor.hpp"
#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {
  template class VoteTrackerImpl<Prevote>;
  template class VoteTrackerImpl<Precommit>;

  template <typename M>
  using VotingMessageT = typename VoteTracker<M>::VotingMessage;

  template <typename M>
  auto VoteTrackerImpl<M>::push(const VotingMessageT<M> &vote, size_t weight) ->
      typename VoteTracker<M>::PushResult {
    auto vote_it = messages_.find(vote.id);
    if (vote_it == messages_.end()) {
      messages_[vote.id] = {vote};
      total_weight_ += weight;
      return PushResult::SUCCESS;
    }
    auto &equivotes = vote_it->second;
    bool isDuplicate = visit_in_place(
        equivotes,
        [&vote](const VotingMessage &voting_message) {
          return voting_message.message.block_hash == vote.message.block_hash;
        },
        [&vote](const EquivocatoryVotingMessage &equivocatory_vote) {
          return equivocatory_vote.first.message.block_hash
                     == vote.message.block_hash
                 or equivocatory_vote.second.message.block_hash
                        == vote.message.block_hash;
        });
    if (not isDuplicate) {
      return visit_in_place(
          equivotes,
          // if there is only single vote for that id, make it equivocatory vote
          [&](const VotingMessage &voting_message) {
            EquivocatoryVotingMessage v;
            v.first = boost::get<VotingMessage>(equivotes);
            v.second = vote;

            messages_[vote.id] = v;
            total_weight_ += weight;
            return PushResult::EQUIVOCATED;
          },
          // otherwise return duplicated
          [&](const EquivocatoryVotingMessage &) {
            return PushResult::DUPLICATED;
          });
    }
    return PushResult::DUPLICATED;
  }

  template <typename M>
  std::vector<typename VoteTracker<M>::VoteVariant>
  VoteTrackerImpl<M>::getMessages() const {
    std::vector<typename VoteTracker<M>::VoteVariant> prevotes;
    // the actual number may be bigger, but it's a good guess
    prevotes.reserve(messages_.size());
    for (const auto &[key, value] : messages_) {
      prevotes.push_back(value);
    }
    return prevotes;
  }

  template <typename M>
  size_t VoteTrackerImpl<M>::totalWeight() const {
    return total_weight_;
  }

}  // namespace kagome::consensus::grandpa
