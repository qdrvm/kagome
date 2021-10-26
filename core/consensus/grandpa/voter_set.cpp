/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/voter_set.hpp"

namespace kagome::consensus::grandpa {

  VoterSet::VoterSet(MembershipCounter id_of_set) : id_{id_of_set} {}

  void VoterSet::insert(Id voter, size_t weight) {
    auto r = map_.emplace(voter, map_.size());
    if (r.second) {
      list_.emplace_back(r.first->first, weight);
      total_weight_ += weight;
    } else {
      BOOST_UNREACHABLE_RETURN();
    }
  }

  boost::optional<Id> VoterSet::voterId(Index index) const {
    if (index >= list_.size()) {
      return boost::none;
    }
    auto voter = std::get<0>(list_[index]);
    return voter;
  }

  boost::optional<std::tuple<VoterSet::Index, VoterSet::Weight>>
  VoterSet::indexAndWeight(const Id &voter) const {
    auto it = map_.find(voter);
    if (it == map_.end()) {
      return boost::none;
    }
    auto index = it->second;
    BOOST_ASSERT(index < list_.size());
    auto weight = std::get<1>(list_[index]);
    return {{index, weight}};
  }

  boost::optional<VoterSet::Index> VoterSet::voterIndex(const Id &voter) const {
    auto it = map_.find(voter);
    if (it == map_.end()) {
      return boost::none;
    }
    auto index = it->second;
    return index;
  }

  boost::optional<VoterSet::Weight> VoterSet::voterWeight(
      const Id &voter) const {
    auto it = map_.find(voter);
    if (it == map_.end()) {
      return boost::none;
    }
    auto index = it->second;
    BOOST_ASSERT(index < list_.size());
    auto weight = std::get<1>(list_[index]);
    return weight;
  }

  boost::optional<VoterSet::Weight> VoterSet::voterWeight(Index index) const {
    if (index >= list_.size()) {
      return boost::none;
    }
    auto weight = std::get<1>(list_.at(index));
    return weight;
  }
}  // namespace kagome::consensus::grandpa
