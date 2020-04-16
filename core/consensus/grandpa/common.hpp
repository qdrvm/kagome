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

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_COMMON_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_COMMON_HPP

#include <unordered_map>

#include "clock/clock.hpp"
#include "common/wrapper.hpp"
#include "crypto/ed25519_types.hpp"
#include "primitives/authority.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::grandpa {

  using Id = crypto::ED25519PublicKey;

  // vote signature
  using Signature = crypto::ED25519Signature;
  using BlockHash = primitives::BlockHash;
  using BlockNumber = primitives::BlockNumber;
  using RoundNumber = uint64_t;
  using MembershipCounter = uint64_t;

  using Clock = clock::SteadyClock;
  using Duration = Clock::Duration;
  using TimePoint = Clock::TimePoint;

  enum class State { START, PROPOSED, PREVOTED, PRECOMMITTED };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_COMMON_HPP
