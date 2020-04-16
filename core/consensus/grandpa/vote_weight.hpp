/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
    // TODO(kamilsa) PRE-358: remove weight
    size_t weight = 0;
    std::vector<size_t> prevotes{kMaxNumberOfVoters, 0UL};
    std::vector<size_t> precommits{kMaxNumberOfVoters, 0UL};
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP
