/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/vote_weight.hpp"

namespace kagome::consensus::grandpa {

  template <typename T>
  VoteWeight::Weight VoteWeight::total(const std::vector<bool> &equivocators,
                                       const VoterSet &voter_set) const {
    Weight result = 0;

    const auto &votes = [&]() {
      if constexpr (std::is_same_v<T, Prevote>) {
        return prevotes;
      } else if constexpr (std::is_same_v<T, Precommit>) {
        return precommits;
      }
    }();

    for (size_t i = voter_set.size(); i > 0;) {
      --i;

      if (equivocators.size() > i and equivocators[i]) {
        result += voter_set.voterWeight(i).value();
      } else if (votes.size() > i and votes[i] != 0) {
        result += votes[i];
      }
    }

    return result;
  }

  template VoteWeight::Weight VoteWeight::total<Prevote>(
      const std::vector<bool> &, const VoterSet &) const;

  template VoteWeight::Weight VoteWeight::total<Precommit>(
      const std::vector<bool> &, const VoterSet &) const;

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
