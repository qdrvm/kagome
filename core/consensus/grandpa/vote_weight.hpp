/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP

#include <numeric>

#include <boost/dynamic_bitset.hpp>
#include <boost/operators.hpp>
#include "consensus/grandpa/structs.hpp"
#include "consensus/grandpa/voter_set.hpp"

namespace kagome::consensus::grandpa {

  class VoteWeight : public boost::equality_comparable<VoteWeight>,
                     public boost::less_than_comparable<VoteWeight> {
   public:
    explicit VoteWeight(size_t voters_size = 256)
        : prevotes(voters_size, 0UL), precommits(voters_size, 0UL) {}

    TotalWeight totalWeight(const std::vector<bool> &prevotes_equivocators,
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

    auto &operator+=(const VoteWeight &vote) {
      for (size_t i = 0; i < prevotes.size() and i < vote.prevotes.size();
           i++) {
        prevotes[i] += vote.prevotes[i];
        precommits[i] += vote.precommits[i];
      }
      weight += vote.weight;
      return *this;
    }

    bool operator==(const VoteWeight &other) const {
      return prevotes == other.prevotes and precommits == other.precommits;
    }
    bool operator<(const VoteWeight &other) const {
      return weight < other.weight;
    }
    size_t weight = 0;
    std::vector<size_t> prevotes{256, 0UL};
    std::vector<size_t> precommits{256, 0UL};

   private:
  };

  inline std::ostream &operator<<(std::ostream &os, const VoteWeight &v) {
    // TODO(warchant): implement
    return os;
  }

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP
