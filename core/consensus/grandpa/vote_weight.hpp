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

  /**
   * Vote weight is a structure that keeps track of who voted for the vote and
   * with which weight
   */
  class VoteWeight : public boost::equality_comparable<VoteWeight>,
                     public boost::less_than_comparable<VoteWeight> {
   public:
    using Weight = size_t;

    std::vector<bool> flags;
    Weight sum = 0;

    void set(size_t index, size_t weight) {
      if (flags.size() <= index) {
        flags.resize(index + 1, 0);
      }
      if (flags[index]) {
        return;
      }
      flags[index] = true;
      sum += weight;
    }

    void unset(size_t index, size_t weight) {
      if (flags.size() <= index) {
        return;
      }
      if (not flags[index]) {
        return;
      }
      flags[index] = false;
      sum -= weight;
    }

    Weight total(const std::vector<bool> &equivocators,
                 const VoterSet &voter_set) const {
      Weight result = sum;

      for (size_t i = voter_set.size(); i > 0;) {
        --i;

        if (not flags[i] and equivocators.size() > i and equivocators[i]) {
          result += voter_set.voterWeight(i).value();
        }
      }

      return result;
    }

    bool operator==(const VoteWeight &other) const {
      return sum == other.sum and flags == other.flags;
    }

    bool operator<(const VoteWeight &other) const {
      return sum < other.sum;
    }
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP
