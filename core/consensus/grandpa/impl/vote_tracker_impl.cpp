/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
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
    bool isDuplicate = std::find_if(equivotes.begin(),
                                    equivotes.end(),
                                    [&vote](auto const &prevote) {
                                      return prevote.message.block_hash
                                             == vote.message.block_hash;
                                    })
                       != equivotes.end();
    if (not isDuplicate && equivotes.size() < 2) {
      equivotes.push_back(vote);
      total_weight_ += weight;
      return PushResult::EQUIVOCATED;
    }
    return PushResult::DUPLICATED;
  }

  template <typename M>
  std::vector<VotingMessageT<M>> VoteTrackerImpl<M>::getMessages() const {
    std::vector<VotingMessageT<M>> prevotes;
    // the actual number may be bigger, but it's a good guess
    prevotes.reserve(messages_.size());
    for (auto const &[_, equivotes] : messages_) {
      prevotes.insert(prevotes.begin(), equivotes.begin(), equivotes.end());
    }
    return prevotes;
  }

  template <typename M>
  size_t VoteTrackerImpl<M>::getTotalWeight() const {
    return total_weight_;
  }

}  // namespace kagome::consensus::grandpa
