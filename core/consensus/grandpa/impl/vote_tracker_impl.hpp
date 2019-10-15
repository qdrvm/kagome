/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_VOTE_TRACKER_IMPL_HPP
#define KAGOME_VOTE_TRACKER_IMPL_HPP

#include "consensus/grandpa/vote_tracker.hpp"

namespace kagome::consensus::grandpa {

  template <typename MessageType>
  class VoteTrackerImpl : public VoteTracker<MessageType> {
   public:
    using PushResult = typename VoteTracker<MessageType>::PushResult;
    using VotingMessage = typename VoteTracker<MessageType>::VotingMessage;

    PushResult push(const VotingMessage &vote, size_t weight) override {
      auto vote_it = messages_.find(vote.id);
      if (vote_it == messages_.end()) {
        messages_[vote.id] = {vote};
        total_weight_ += weight;
        return PushResult::SUCCESS;
      }
      auto &equivotes = vote_it->second;
      bool isDuplicate =
          std::find_if(equivotes.begin(),
                       equivotes.end(),
                       [&vote](auto const &prevote) {
                         return prevote.message.hash == vote.message.hash;
                       })
          != equivotes.end();
      if (not isDuplicate && equivotes.size() < 2) {
        equivotes.push_back(vote);
        total_weight_ += weight;
        return PushResult::EQUIVOCATED;
      }
      return PushResult::DUPLICATED;
    }

    std::vector<VotingMessage> getMessages() const override {
      std::vector<SignedPrevote> prevotes;
      // the actual number may be bigger, but it's a good guess
      prevotes.reserve(messages_.size());
      for (auto const &[_, equivotes] : messages_) {
        prevotes.insert(prevotes.begin(), equivotes.begin(), equivotes.end());
      }
      return prevotes;
    }

    size_t getTotalWeight() const override {
      return total_weight_;
    }

   private:
    std::map<Id, std::list<VotingMessage>> messages_;
    size_t total_weight_ = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_VOTE_TRACKER_IMPL_HPP
