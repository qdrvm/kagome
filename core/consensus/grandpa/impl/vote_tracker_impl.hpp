/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
    using EquivocatoryVotingMessage =
        typename VoteTracker<MessageType>::EquivocatoryVotingMessage;
    using VoteVariant = typename VoteTracker<MessageType>::VoteVariant;

    ~VoteTrackerImpl() override = default;

    PushResult push(const VotingMessage &vote, size_t weight) override;

    std::vector<VoteVariant> getMessages() const override;

    size_t getTotalWeight() const override;

   private:
    std::map<Id, VoteVariant> messages_;
    size_t total_weight_ = 0;
  };

  using PrevoteTrackerImpl = VoteTrackerImpl<Prevote>;
  using PrecommitTrackerImpl = VoteTrackerImpl<Precommit>;

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_VOTE_TRACKER_IMPL_HPP
