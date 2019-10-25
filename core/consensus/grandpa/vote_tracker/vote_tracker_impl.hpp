/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_TRACKER_VOTE_TRACKER_IMPL_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_TRACKER_VOTE_TRACKER_IMPL_HPP

#include "consensus/grandpa/vote_tracker.hpp"

#include <boost/range/adaptors.hpp>

namespace kagome::consensus::grandpa {

  class VoteTrackerImpl : public VoteTracker {
   public:
    ~VoteTrackerImpl() override = default;

    PushResult push(const SignedPrevote &vote, size_t weight) override {
      return push(vote,
                  weight,
                  total_prevotes_,
                  single_prevotes_,
                  equivocated_prevotes_);
    }

    PushResult push(const SignedPrecommit &vote, size_t weight) override {
      return push(vote,
                  weight,
                  total_precommits_,
                  single_precommits_,
                  equivocated_precommits_);
    }

    std::vector<SignedPrevote> getPrevotes() const override {
      std::vector<SignedPrevote> result;

      for (const auto &[_, vote_weight] : single_prevotes_) {
        result.push_back(vote_weight.first);
      }
      for (const auto &[_, vote_weight_pair] : equivocated_prevotes_) {
        result.push_back(vote_weight_pair.first.first);
        result.push_back(vote_weight_pair.second.first);
      }

      return result;
    }
    std::vector<SignedPrecommit> getPrecommits() const override {
      std::vector<SignedPrecommit> result;

      for (const auto &[_, vote_weight] : single_precommits_) {
        result.push_back(vote_weight.first);
      }
      for (const auto &[_, vote_weight_pair] : equivocated_precommits_) {
        result.push_back(vote_weight_pair.first.first);
        result.push_back(vote_weight_pair.second.first);
      }

      return result;
    }

    size_t prevoteWeight() const override {
      return total_prevotes_;
    }
    size_t precommitWeight() const override {
      return total_precommits_;
    }

    Justification getJustification(const BlockInfo &info) const override {
      return {};
    }

   private:
    template <typename SignedVoteType>
    using VoteWeight = std::pair<SignedVoteType, size_t>;

    using PrevoteWeight = VoteWeight<SignedPrevote>;
    using PrecommitWeight = VoteWeight<SignedPrecommit>;

    template <typename VoteWeightType>
    using SingleVotesMap = std::unordered_map<Id, VoteWeightType>;

    template <typename VoteWeightType>
    using EquivocatedVotesMap =
        std::unordered_map<Id, std::pair<VoteWeightType, VoteWeightType>>;

    template <typename SignedVoteType>
    PushResult push(
        const SignedVoteType &vote,
        size_t weight,
        size_t &total_weight,
        SingleVotesMap<VoteWeight<SignedVoteType>> &single_votes_map,
        EquivocatedVotesMap<VoteWeight<SignedVoteType>>
            &equivocated_votes_map) {
      auto it = single_votes_map.find(vote.id);
      if (it == single_votes_map.end()) {
        single_votes_map.insert({vote.id, {vote, weight}});
        total_weight += weight;
        return PushResult::SUCCESS;
      }

      if (it->second.first.message == vote.message) {
        return PushResult::DUPLICATE;
      }
      // if got here, then it is equivocated vote. Then we remove vote from
      // single votes map and insert into equivocated map
      equivocated_votes_map.insert({vote.id, {it->second, {vote, weight}}});
      single_votes_map.erase(it);
      total_weight += weight;
      return PushResult::EQUIVOCATED;
    }

    SingleVotesMap<PrevoteWeight> single_prevotes_;
    EquivocatedVotesMap<PrevoteWeight> equivocated_prevotes_;
    size_t total_prevotes_;

    SingleVotesMap<PrecommitWeight> single_precommits_;
    EquivocatedVotesMap<PrecommitWeight> equivocated_precommits_;
    size_t total_precommits_;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_TRACKER_VOTE_TRACKER_IMPL_HPP
