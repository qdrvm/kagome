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

    virtual ~VoteTrackerImpl() = default;

    PushResult push(const VotingMessage &vote, size_t weight) override;

    std::vector<VotingMessage> getMessages() const override;

    size_t getTotalWeight() const override;

   private:
    std::map<Id, std::list<VotingMessage>> messages_;
    size_t total_weight_ = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_VOTE_TRACKER_IMPL_HPP
