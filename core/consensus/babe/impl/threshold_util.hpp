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

#ifndef KAGOME_CORE_CONSENSUS_BABE_IMPL_THRESHOLD_UTIL_HPP
#define KAGOME_CORE_CONSENSUS_BABE_IMPL_THRESHOLD_UTIL_HPP

#include "consensus/babe/common.hpp"
#include "primitives/authority.hpp"

namespace kagome::consensus {

  /// Calculates the primary selection threshold for a given authority, taking
  /// into account `c` (`1 - c` represents the probability of a slot being
  /// empty).
  /// https://github.com/paritytech/substrate/blob/7010ec7716e0edf97d61a29bd0c337648b3a57ae/core/consensus/babe/src/authorship.rs#L30
  Threshold calculateThreshold(
      const std::pair<uint64_t, uint64_t> &c_pair,
      const std::vector<primitives::Authority> &authorities,
      primitives::AuthorityIndex authority_index);

}  // namespace kagome::consensus

#endif  // KAGOME_CORE_CONSENSUS_BABE_IMPL_THRESHOLD_UTIL_HPP
