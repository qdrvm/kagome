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
  size_t VoteTrackerImpl<M>::getTotalWeight() const {
    return total_weight_;
  }

}  // namespace kagome::consensus::grandpa
