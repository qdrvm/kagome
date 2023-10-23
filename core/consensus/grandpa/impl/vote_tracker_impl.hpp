/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
