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
    using EquivocatoryVotingMessage = typename VoteTracker<MessageType>::EquivocatoryVotingMessage ;
    using VoteVariant = typename VoteTracker<MessageType>::VoteVariant ;

    virtual ~VoteTrackerImpl() = default;

    PushResult push(const VotingMessage &vote, size_t weight) override;

    std::vector<VoteVariant> getMessages() const override;

    size_t totalWeight() const override;

   private:
    std::map<Id, VoteVariant> messages_;
    size_t total_weight_ = 0;
  };

  using PrevoteTrackerImpl = VoteTrackerImpl<Prevote>;
  using PrecommitTrackerImpl = VoteTrackerImpl<Precommit>;

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_VOTE_TRACKER_IMPL_HPP
