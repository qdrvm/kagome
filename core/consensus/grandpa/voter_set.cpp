/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/voter_set.hpp"

namespace kagome::consensus::grandpa {

  VoterSet::VoterSet(MembershipCounter id_of_set) : id_{id_of_set} {}

  void VoterSet::insert(Id voter, size_t weight) {
    voters_.push_back(voter);
    weight_map_.insert({voter, weight});
    total_weight_ += weight;
  }

  boost::optional<size_t> VoterSet::voterIndex(const Id &voter) const {
    auto it = std::find(voters_.begin(), voters_.end(), voter);
    if (it == voters_.end()) {
      return boost::none;
    }
    return std::distance(voters_.begin(), it);
  }

  boost::optional<size_t> VoterSet::voterWeight(const Id &voter) const {
    auto it = weight_map_.find(voter);
    if (it == weight_map_.end()) {
      return boost::none;
    }
    return it->second;
  }

  boost::optional<size_t> VoterSet::voterWeight(size_t voter_index) const {
    if (voter_index >= voters_.size()) {
      return boost::none;
    }
    return weight_map_.at(voters_[voter_index]);
  }

}  // namespace kagome::consensus::grandpa
