/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "common/outcome_throw.hpp"
#include "consensus/grandpa/common.hpp"
#include "consensus/grandpa/types/authority.hpp"
#include "consensus/timeline/types.hpp"
#include "scale/kagome_scale.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Stores voters with their corresponding weights
   */
  class VoterSet final {
   public:
    enum class Error { VOTER_ALREADY_EXISTS = 1, INDEX_OUTBOUND };

    using Index = AuthorityIndex;
    using Weight = AuthorityWeight;

    VoterSet() = default;  // for scale codec (in decode)

    explicit VoterSet(VoterSetId id_of_set);

    static outcome::result<std::shared_ptr<VoterSet>> make(
        const AuthoritySet &voters);

    /**
     * Insert voter \param voter with \param weight
     */
    outcome::result<void> insert(Id voter, Weight weight);

    /**
     * \return unique voter set membership id
     */
    inline const VoterSetId &id() const {
      return id_;
    }

    std::optional<std::tuple<Index, Weight>> indexAndWeight(
        const Id &voter) const;

    outcome::result<Id> voterId(Index index) const;

    /**
     * \return index of \param voter
     */
    std::optional<Index> voterIndex(const Id &voter) const;

    /**
     * \return weight of \param voter
     */
    std::optional<Weight> voterWeight(const Id &voter) const;

    /**
     * \return weight of voter by index \param voter_index
     */
    outcome::result<Weight> voterWeight(Index voter_index) const;

    inline size_t size() const {
      return list_.size();
    }

    inline bool empty() const {
      return list_.empty();
    }

    /**
     * \return total weight of all voters
     */
    inline Weight totalWeight() const {
      return total_weight_;
    }

   private:
    VoterSetId id_{};
    std::unordered_map<Id, Index> map_;
    std::vector<std::tuple<Id, Weight>> list_;
    size_t total_weight_{0};

    friend void encode(const VoterSet &voters, scale::Encoder &encoder) {
      encode(std::tie(voters.list_, voters.id_), encoder);
    }

    friend void decode(VoterSet &voters, scale::Decoder &decoder) {
      voters.list_.clear();
      voters.map_.clear();
      voters.total_weight_ = 0;

      std::vector<std::tuple<Id, VoterSet::Weight>> list;
      decode(std::tie(list, voters.id_), decoder);
      for (const auto &[id, weight] : list) {
        auto r = voters.insert(id, weight);
        if (r.has_error()) {
          common::raise(r.error());
        }
      }
    }
  };

}  // namespace kagome::consensus::grandpa

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::grandpa, VoterSet::Error);
