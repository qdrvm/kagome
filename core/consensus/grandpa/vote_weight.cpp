/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/vote_weight.hpp"

namespace kagome::consensus::grandpa {

  VoteWeight::VoteWeight(size_t voters_size)
      : prevotes(voters_size, 0UL), precommits(voters_size, 0UL){};

  TotalWeight VoteWeight::totalWeight(
      const std::vector<bool> &prevotes_equivocators,
      const std::vector<bool> &precommits_equivocators,
      const std::shared_ptr<VoterSet> &voter_set) const {
    std::vector<size_t> prevotes_weight_with_equivocators(prevotes);
    std::vector<size_t> precommits_weight_with_equivocators(precommits);

    for (size_t i = 0; i < voter_set->size(); i++) {
      if (prevotes[i] == 0 and prevotes_equivocators[i]) {
        prevotes_weight_with_equivocators[i] +=
            voter_set->voterWeight(i).value();
      }
      if (precommits[i] == 0 and precommits_equivocators[i]) {
        precommits_weight_with_equivocators[i] +=
            voter_set->voterWeight(i).value();
      }
    }

    TotalWeight weight{
        .prevote = std::accumulate(prevotes_weight_with_equivocators.begin(),
                                   prevotes_weight_with_equivocators.end(),
                                   0UL),
        .precommit =
            std::accumulate(precommits_weight_with_equivocators.begin(),
                            precommits_weight_with_equivocators.end(),
                            0UL)};
    return weight;
  }

  VoteWeight &VoteWeight::operator+=(const VoteWeight &vote) {
    for (size_t i = 0; i < prevotes.size() and i < vote.prevotes.size(); i++) {
      prevotes[i] += vote.prevotes[i];
      precommits[i] += vote.precommits[i];
    }
    weight += vote.weight;
    return *this;
  }
}  // namespace kagome::consensus::grandpa
