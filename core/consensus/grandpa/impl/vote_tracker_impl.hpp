/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_VOTE_TRACKER_IMPL_HPP
#define KAGOME_VOTE_TRACKER_IMPL_HPP

#include "consensus/grandpa/vote_tracker.hpp"

namespace kagome::consensus::grandpa {

  class VoteTrackerImpl : public VoteTracker {
   public:
    PushResult push(const SignedMessage &vote, size_t weight) override;

    void unpush(const SignedMessage &vote, size_t weight) override;

    std::vector<VoteVariant> getMessages() const override;

    size_t getTotalWeight() const override;

   private:
    std::map<Id, VoteVariant> messages_;
    size_t total_weight_ = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_VOTE_TRACKER_IMPL_HPP
