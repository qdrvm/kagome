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

#ifndef KAGOME_COMMON_HPP
#define KAGOME_COMMON_HPP

#include <cstdint>

#include "clock/clock.hpp"
#include "crypto/sr25519_types.hpp"

namespace kagome::consensus {
  using BabeClock = clock::SystemClock;

  /// BABE uses system clock's time points
  using BabeTimePoint = BabeClock::TimePoint;

  // Babe uses system clock's duration
  using BabeDuration = BabeClock::Duration;

  /// slot number of the Babe production
  using BabeSlotNumber = uint64_t;

  /// number of the epoch in the Babe production
  using EpochIndex = uint64_t;

  // number of slots in a single epoch
  using EpochLength = EpochIndex;

  /// threshold, which must not be exceeded for the party to be a slot leader
  using Threshold = crypto::VRFThreshold;

  /// random value, which serves as a seed for VRF slot leadership selection
  using Randomness = common::Blob<crypto::constants::sr25519::vrf::OUTPUT_SIZE>;
}  // namespace kagome::consensus

#endif  // KAGOME_COMMON_HPP
