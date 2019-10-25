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

namespace kagome::consensus::grandpa {

  class VoteWeight : public boost::equality_comparable<VoteWeight>,
                     public boost::less_than_comparable<VoteWeight> {
   public:
    VoteWeight(size_t bits_size = 256)
        : prevotes(bits_size, 0UL), precommits(bits_size, 0UL) {}

    TotalWeight totalWeight() const {
      TotalWeight weight{
          .prevote = std::accumulate(prevotes.begin(), prevotes.end(), 0UL),
          .precommit =
              std::accumulate(precommits.begin(), precommits.end(), 0UL)};
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
