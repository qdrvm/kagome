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

  inline const size_t kMaxNumberOfVoters = 256;

  /**
   * Vote weight is a structure that keeps track of who voted for the vote and
   * with which weight
   */
  class VoteWeight : public boost::equality_comparable<VoteWeight>,
                     public boost::less_than_comparable<VoteWeight> {
   public:
    explicit VoteWeight(size_t voters_size = kMaxNumberOfVoters);

    /**
     * Get total weight of current vote's weight
     * @param prevotes_equivocators describes peers which equivocated (voted
     * twice for different block) during prevote. Index in vector corresponds to
     * the authority index of the peer. Bool is true if peer equivocated, false
     * otherwise
     * @param precommits_equivocators same for precommits
     * @param voter_set list of peers with their weight
     * @return totol weight of current vote's weight
     */
    TotalWeight totalWeight(const std::vector<bool> &prevotes_equivocators,
                            const std::vector<bool> &precommits_equivocators,
                            const std::shared_ptr<VoterSet> &voter_set) const;

    VoteWeight &operator+=(const VoteWeight &vote);

    bool operator==(const VoteWeight &other) const {
      return prevotes == other.prevotes and precommits == other.precommits;
    }
    bool operator<(const VoteWeight &other) const {
      return weight < other.weight;
    }
    size_t weight = 0;
    std::vector<size_t> prevotes{kMaxNumberOfVoters, 0UL};
    std::vector<size_t> precommits{kMaxNumberOfVoters, 0UL};
  };

  inline std::ostream &operator<<(std::ostream &os, const VoteWeight &v) {
    // TODO(warchant): implement
    return os;
  }

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP
