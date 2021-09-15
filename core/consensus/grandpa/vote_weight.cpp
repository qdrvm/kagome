/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/vote_weight.hpp"

namespace kagome::consensus::grandpa {

  TotalWeight VoteWeight::totalWeight(
      const std::vector<bool> &prevotes_equivocators,
      const std::vector<bool> &precommits_equivocators,
      const std::shared_ptr<VoterSet> &voter_set) const {
    TotalWeight weight;

    for (size_t i = voter_set->size(); i > 0;) {
      --i;

      if (prevotes.size() > i and prevotes[i] != 0) {
        weight.prevote += prevotes[i];
      } else if (prevotes_equivocators.size() > i
                 and prevotes_equivocators[i]) {
        weight.prevote += voter_set->voterWeight(i).value();
      }

      if (precommits.size() > i and precommits[i] != 0) {
        weight.precommit += precommits[i];
      } else if (precommits_equivocators.size() > i
                 and precommits_equivocators[i]) {
        weight.precommit += voter_set->voterWeight(i).value();
      }
    }

    return weight;
  }

  VoteWeight &VoteWeight::operator+=(const VoteWeight &vote) {
    for (size_t i = vote.prevotes.size(); i > 0;) {
      if (prevotes.size() < i) {
        prevotes.resize(i, 0);
      }
      --i;
      prevotes[i] += vote.prevotes[i];
    }
    prevotes_sum += vote.prevotes_sum;

    for (size_t i = vote.precommits.size(); i > 0;) {
      if (precommits.size() < i) {
        precommits.resize(i, 0);
      }
      --i;
      precommits[i] += vote.precommits[i];
    }
    precommits_sum += vote.precommits_sum;

    return *this;
  }
}  // namespace kagome::consensus::grandpa
