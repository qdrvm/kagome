/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/voter_set.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::grandpa, VoterSet::Error, e) {
  using E = kagome::consensus::grandpa::VoterSet::Error;
  switch (e) {
    case E::VOTER_ALREADY_EXISTS:
      return "Voter already exists";
    case E::VOTER_NOT_FOUND:
      return "Voter not found";
    case E::INDEX_OUTBOUND:
      return "Index outbound";
    case E::QUERYING_ZERO_VOTER:
      return "Weight of a voter with a zero public key is queried";
  }
  return "Unknown error (invalid VoterSet::Error)";
}

namespace kagome::consensus::grandpa {

  VoterSet::VoterSet(VoterSetId id_of_set) : id_{id_of_set} {}

  outcome::result<void> VoterSet::insert(Id voter, size_t weight) {
    // zero authorities break the mapping logic a bit, but since they must not
    // be queried it should be fine
    if (voter == Id{}) {
      list_.emplace_back(voter, weight);
      total_weight_ += weight;
      return outcome::success();
    }
    auto r = map_.emplace(voter, map_.size());
    if (r.second) {
      list_.emplace_back(r.first->first, weight);
      total_weight_ += weight;
      return outcome::success();
    }
    return Error::VOTER_ALREADY_EXISTS;
  }

  outcome::result<Id> VoterSet::voterId(Index index) const {
    if (index >= list_.size()) {
      return Error::INDEX_OUTBOUND;
    }
    auto voter = std::get<0>(list_[index]);
    return voter;
  }

  outcome::result<std::tuple<VoterSet::Index, VoterSet::Weight>>
  VoterSet::indexAndWeight(const Id &voter) const {
    if (voter == Id{}) {
      return Error::QUERYING_ZERO_VOTER;
    }
    auto it = map_.find(voter);
    if (it == map_.end()) {
      return Error::VOTER_NOT_FOUND;
    }
    auto index = it->second;
    BOOST_ASSERT(index < list_.size());
    auto weight = std::get<1>(list_[index]);
    return std::tuple(index, weight);
  }

  outcome::result<VoterSet::Index> VoterSet::voterIndex(const Id &voter) const {
    if (voter == Id{}) {
      return Error::QUERYING_ZERO_VOTER;
    }
    auto it = map_.find(voter);
    if (it == map_.end()) {
      return Error::VOTER_NOT_FOUND;
    }
    auto index = it->second;
    return index;
  }

  outcome::result<VoterSet::Weight> VoterSet::voterWeight(
      const Id &voter) const {
    if (voter == Id{}) {
      return Error::QUERYING_ZERO_VOTER;
    }
    auto it = map_.find(voter);
    if (it == map_.end()) {
      return Error::VOTER_NOT_FOUND;
    }
    auto index = it->second;
    BOOST_ASSERT(index < list_.size());
    auto weight = std::get<1>(list_[index]);
    return weight;
  }

  outcome::result<VoterSet::Weight> VoterSet::voterWeight(Index index) const {
    if (index >= list_.size()) {
      return Error::INDEX_OUTBOUND;
    }
    auto weight = std::get<1>(list_.at(index));
    return weight;
  }
}  // namespace kagome::consensus::grandpa
