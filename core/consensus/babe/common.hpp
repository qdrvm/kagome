/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_HPP
#define KAGOME_COMMON_HPP

#include <array>
#include <cstdint>

#include "clock/clock.hpp"
#include "crypto/sr25519_types.hpp"

namespace kagome::consensus {
  /// BABE uses system clock's time points
  using BabeTimePoint = clock::SystemClock::TimePoint;

  /// slot number of the Babe production
  using BabeSlotNumber = uint64_t;

  /// number of the epoch in the Babe production
  using EpochIndex = uint64_t;

  /// threshold, which must not be exceeded for the party to be a slot leader
  using Threshold = crypto::VRFValue;

  /// random value, which serves as a seed for VRF slot leadership selection
  using Randomness = std::array<uint8_t, SR25519_VRF_OUTPUT_SIZE>;
}  // namespace kagome::consensus

#endif  // KAGOME_COMMON_HPP
