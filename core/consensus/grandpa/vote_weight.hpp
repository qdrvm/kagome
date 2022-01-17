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

   private:
    struct ForType {
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

      void merge(const ForType &other,
                 const std::shared_ptr<VoterSet> &voter_set) {
        for (auto i = other.flags.size(); i > 0;) {
          --i;
          if (other.flags[i]) {
            set(i, voter_set->voterWeight(i).value());
          }
        }
      }

      bool operator==(const ForType &other) const {
        return sum == other.sum and flags == other.flags;
      }
    };

    ForType pv;
    ForType pc;

   public:
    inline Weight sum(VoteType vote_type) const {
      switch (vote_type) {
        case VoteType::Prevote:
          return pv.sum;
        case VoteType::Precommit:
          return pc.sum;
      }
      BOOST_UNREACHABLE_RETURN({});
    }

    inline void set(VoteType vote_type, size_t index, size_t weight) {
      switch (vote_type) {
        case VoteType::Prevote:
          return pv.set(index, weight);
        case VoteType::Precommit:
          return pc.set(index, weight);
      }
      BOOST_UNREACHABLE_RETURN();
    }

    inline void unset(VoteType vote_type, size_t index, size_t weight) {
      switch (vote_type) {
        case VoteType::Prevote:
          return pv.unset(index, weight);
        case VoteType::Precommit:
          return pc.unset(index, weight);
      }
      BOOST_UNREACHABLE_RETURN();
    }

    inline Weight total(VoteType vote_type,
                        const std::vector<bool> &equivocators,
                        const VoterSet &voter_set) const {
      switch (vote_type) {
        case VoteType::Prevote:
          return pv.total(equivocators, voter_set);
        case VoteType::Precommit:
          return pc.total(equivocators, voter_set);
      }
      BOOST_UNREACHABLE_RETURN({});
    }

    void merge(const VoteWeight &other,
               const std::shared_ptr<VoterSet> &voter_set) {
      pv.merge(other.pv, voter_set);
      pc.merge(other.pc, voter_set);
    }

    bool operator==(const VoteWeight &other) const {
      return pv == other.pv and pc == other.pc;
    }
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_WEIGHT_HPP
