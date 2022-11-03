/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTER_SET_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTER_SET_HPP

#include <optional>

#include "common/outcome_throw.hpp"
#include "consensus/grandpa/common.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Stores voters with their corresponding weights
   */
  struct VoterSet final {
   public:
    enum class Error {
      VOTER_ALREADY_EXISTS = 1,
      INDEX_OUTBOUND
    };

    using Index = size_t;
    using Weight = size_t;

    VoterSet() = default;  // for scale codec (in decode)

    explicit VoterSet(VoterSetId id_of_set);

    /**
     * Insert voter \param voter with \param weight
     */
    outcome::result<void> insert(Id voter, Weight weight);

    /**
     * \return unique voter set membership id
     */
    inline VoterSetId id() const {
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
    std::vector<std::tuple<const Id &, Weight>> list_;
    size_t total_weight_{0};

    template <class Stream>
    friend Stream &operator<<(Stream &s, const VoterSet &voters);
    template <class Stream>
    friend Stream &operator>>(Stream &s, VoterSet &voters);
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const VoterSet &voters) {
    return s << voters.list_ << voters.id_;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, VoterSet &voters) {
    voters.list_.clear();
    voters.map_.clear();
    voters.total_weight_ = 0;

    std::vector<std::tuple<Id, VoterSet::Weight>> list;
    s >> list >> voters.id_;
    for (const auto &[id, weight] : list) {
      auto r = voters.insert(id, weight);
      if (r.has_error()) {
        common::raise(r.as_failure());
      }
    }
    return s;
  }

}  // namespace kagome::consensus::grandpa

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::grandpa, VoterSet::Error);

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTER_SET_HPP
