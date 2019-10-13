/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP

#include <boost/dynamic_bitset.hpp>
#include <boost/operators.hpp>
#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  struct VoteWeight : public boost::equality_comparable<VoteWeight>,
                      public boost::less_than_comparable<VoteWeight> {
    explicit VoteWeight(size_t bits_size = 256)
        : prevotes(bits_size), precommits(bits_size) {}

    boost::dynamic_bitset<> prevotes;
    boost::dynamic_bitset<> precommits;

    TotalWeight totalWeight() const {
      return TotalWeight{.prevote = prevotes.count(),
                         .precommit = precommits.count()};
    }

    // TODO(warchant): change size_t to something else, but preserve all
    // operators
    size_t weight = 0;

    auto &operator+=(const VoteWeight &vote) {
      weight += vote.weight;
      return *this;
    }

    bool operator==(const VoteWeight &other) const {
      return weight == other.weight;
    }

    bool operator<(const VoteWeight &other) const {
      return weight < other.weight;
    }
  };

  inline std::ostream &operator<<(std::ostream &os, const VoteWeight &v) {
    // TODO(warchant): implement
    return os << v.weight;
  }

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP
