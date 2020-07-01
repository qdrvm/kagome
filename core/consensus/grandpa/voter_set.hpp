/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTER_SET_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTER_SET_HPP

#include <boost/optional.hpp>

#include "consensus/grandpa/common.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Stores voters with their correspondend weights
   */
  struct VoterSet {
   public:
    VoterSet() = default;  // for scale codec (in decode)

    explicit VoterSet(MembershipCounter id_of_set);

    /**
     * Insert voter \param voter with \param weight
     */
    void insert(Id voter, size_t weight);

    /**
     * \return voters
     */
    inline const std::vector<Id> &voters() const {
      return voters_;
    }

    /**
     * \return uniqie voter set membership
     */
    inline MembershipCounter id() const {
      return id_;
    }

    /**
     * \return index of \param voter
     */
    boost::optional<size_t> voterIndex(const Id &voter) const;

    /**
     * \return weight of \param voter
     */
    boost::optional<size_t> voterWeight(const Id &voter) const;

    /**
     * \return weight of voter by index \param voter_index
     */
    boost::optional<size_t> voterWeight(size_t voter_index) const;

    inline size_t size() const {
      return voters_.size();
    }

    /**
     * \return total weight of all voters
     */
    inline size_t totalWeight() const {
      return total_weight_;
    }

    /**
     * \return map of pairs <id, weight>
     */
    inline const std::unordered_map<Id, size_t> &weightMap() const {
      return weight_map_;
    }

   private:
    std::vector<Id> voters_;
    MembershipCounter id_{};
    std::unordered_map<Id, size_t> weight_map_;
    size_t total_weight_{0};
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const VoterSet &voters) {
    std::vector<std::pair<Id, size_t>> key_val_vector;
    key_val_vector.reserve(voters.weightMap().size());
    for (const auto &[id, weight] : voters.weightMap()) {
      key_val_vector.emplace_back(id, weight);
    }
    return s << key_val_vector << voters.id();
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, VoterSet &voters) {
    std::vector<std::pair<Id, size_t>> key_val_vector;
    MembershipCounter set_id = 0;
    s >> key_val_vector >> set_id;
    VoterSet voter_set{set_id};
    for (const auto &[id, weight] : key_val_vector) {
      voter_set.insert(id, weight);
    }
    voters = voter_set;
    return s;
  }

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTER_SET_HPP
