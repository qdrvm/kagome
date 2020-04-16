/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "consensus/grandpa/voter_set.hpp"

namespace kagome::consensus::grandpa {

  VoterSet::VoterSet(MembershipCounter set_id) : set_id_{set_id} {}

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
