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
#include "consensus/grandpa/vote_types.hpp"
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

    struct OneTypeVoteWeight {
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

          if ((flags.size() <= i or not flags[i]) and equivocators.size() > i
              and equivocators[i]) {
            result += voter_set.voterWeight(i).value();
          }
        }

        return result;
      }

      void merge(const OneTypeVoteWeight &other,
                 const std::shared_ptr<VoterSet> &voter_set) {
        for (auto i = other.flags.size(); i > 0;) {
          --i;
          if (other.flags[i]) {
            set(i, voter_set->voterWeight(i).value());
          }
        }
      }

      bool operator==(const OneTypeVoteWeight &other) const {
        return sum == other.sum and flags == other.flags;
      }
    };

    Weight sum(VoteType vote_type) const {
      switch (vote_type) {
        case VoteType::Prevote:
          return prevotes_weight.sum;
        case VoteType::Precommit:
          return precommits_weight.sum;
      }
      BOOST_UNREACHABLE_RETURN({});
    }

    void set(VoteType vote_type, size_t index, size_t weight) {
      switch (vote_type) {
        case VoteType::Prevote:
          return prevotes_weight.set(index, weight);
        case VoteType::Precommit:
          return precommits_weight.set(index, weight);
      }
      BOOST_UNREACHABLE_RETURN();
    }

    void unset(VoteType vote_type, size_t index, size_t weight) {
      switch (vote_type) {
        case VoteType::Prevote:
          return prevotes_weight.unset(index, weight);
        case VoteType::Precommit:
          return precommits_weight.unset(index, weight);
      }
      BOOST_UNREACHABLE_RETURN();
    }

    Weight total(VoteType vote_type,
                 const std::vector<bool> &equivocators,
                 const VoterSet &voter_set) const {
      switch (vote_type) {
        case VoteType::Prevote:
          return prevotes_weight.total(equivocators, voter_set);
        case VoteType::Precommit:
          return precommits_weight.total(equivocators, voter_set);
      }
      BOOST_UNREACHABLE_RETURN({});
    }

    void merge(const VoteWeight &other,
               const std::shared_ptr<VoterSet> &voter_set) {
      prevotes_weight.merge(other.prevotes_weight, voter_set);
      precommits_weight.merge(other.precommits_weight, voter_set);
    }

    bool operator==(const VoteWeight &other) const {
      return prevotes_weight == other.prevotes_weight
             and precommits_weight == other.precommits_weight;
    }

   private:
    OneTypeVoteWeight prevotes_weight;
    OneTypeVoteWeight precommits_weight;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP
