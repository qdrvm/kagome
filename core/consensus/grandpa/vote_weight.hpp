/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP

#include <boost/dynamic_bitset.hpp>
#include <boost/operators.hpp>

namespace kagome::consensus::grandpa {

  struct VoteWeight : public boost::equality_comparable<VoteWeight>,
                      public boost::less_than_comparable<VoteWeight> {
    // TODO(warchant): change size_t to something else, but preserve all operators 
    size_t weight = 0;

    auto &operator+=(const VoteWeight &vote) {
      weight += vote.weight;
      return *this;
    }

    bool operator==(const VoteWeight &other) const {
      return weight == other.weight;
    }

    bool operator<(const VoteWeight other) const {
      return weight < other.weight;
    }
  };

  inline std::ostream& operator << (std::ostream& os, const VoteWeight& v)  {
    // TODO(warchant): implement
    return os << v.weight;
  }

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP
