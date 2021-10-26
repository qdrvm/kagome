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

    template <typename T>
    Weight total(const std::vector<bool> &equivocators,
                 const VoterSet &voter_set) const;

    VoteWeight &operator+=(const VoteWeight &vote);

    bool operator==(const VoteWeight &other) const {
      return prevotes_sum == other.prevotes_sum
             and precommits_sum == other.precommits_sum
             and prevotes == other.prevotes and precommits == other.precommits;
    }

    size_t prevotes_sum = 0;
    size_t precommits_sum = 0;

    std::vector<size_t> prevotes;
    std::vector<size_t> precommits;

    void setPrevote(size_t index, size_t weight) {
      if (prevotes.size() <= index) {
        prevotes.resize(index + 1, 0);
      }
      prevotes_sum -= prevotes[index];
      prevotes[index] = weight;
      prevotes_sum += weight;
    }

    void setPrecommit(size_t index, size_t weight) {
      if (precommits.size() <= index) {
        precommits.resize(index + 1, 0);
      }
      precommits_sum -= precommits[index];
      precommits[index] = weight;
      precommits_sum += weight;
    }

    static inline const struct {
      bool operator()(const VoteWeight &lhs, const VoteWeight &rhs) {
        return lhs.prevotes_sum < rhs.prevotes_sum;
      }
    } prevoteComparator;

    static inline const struct {
      bool operator()(const VoteWeight &lhs, const VoteWeight &rhs) {
        return lhs.precommits_sum < rhs.precommits_sum;
      }
    } precommitComparator;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP
