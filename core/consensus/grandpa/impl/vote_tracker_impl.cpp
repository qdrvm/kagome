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
    auto &equivotes = vote_it->second;
    bool isDuplicate = visit_in_place(
        equivotes,
        [&vote](const SignedMessage &voting_message) {
          return voting_message.getBlockHash() == vote.getBlockHash();
        },
        [&vote](const EquivocatorySignedMessage &equivocatory_vote) {
          return equivocatory_vote.first.getBlockHash() == vote.getBlockHash()
                 or equivocatory_vote.second.getBlockHash()
                        == vote.getBlockHash();
        });
    if (not isDuplicate) {
      return visit_in_place(
          equivotes,
          // if there is only single vote for that id, make it equivocatory vote
          [&](const SignedMessage &voting_message) {
            EquivocatorySignedMessage v;
            v.first = boost::get<SignedMessage>(equivotes);
            v.second = vote;

            messages_[vote.id] = v;
            total_weight_ += weight;
            equivocators_weight_ += weight;
            return PushResult::EQUIVOCATED;
          },
          // otherwise return duplicated
          [&](const EquivocatorySignedMessage &) {
            return PushResult::DUPLICATED;
          });
    }
    return PushResult::DUPLICATED;
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
    std::vector<VoteVariant> prevotes;
    // the actual number may be bigger, but it's a good guess
    prevotes.reserve(messages_.size());
    for (const auto &[key, value] : messages_) {
      prevotes.push_back(value);
    }
    return prevotes;
  }

  size_t VoteTrackerImpl::getTotalWeight() const {
    return total_weight_;
  }

  size_t VoteTrackerImpl::getEquivocatorsWeight() const {
    return equivocators_weight_;
  }

}  // namespace kagome::consensus::grandpa
