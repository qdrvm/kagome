/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_HPP
#define KAGOME_COMMON_HPP

#include <cstdint>

#include "clock/clock.hpp"
#include "crypto/sr25519_types.hpp"

namespace kagome::consensus::babe {

  using BabeClock = clock::SystemClock;

  /// BABE uses system clock's time points
  using BabeTimePoint = BabeClock::TimePoint;

  // Babe uses system clock's duration
  using BabeDuration = BabeClock::Duration;

  /// slot number of the Babe production
  using BabeSlotNumber = uint64_t;

  /// number of the epoch in the Babe production
  using EpochNumber = uint64_t;

  // number of slots in a single epoch
  using EpochLength = EpochNumber;

  /// threshold, which must not be exceeded for the party to be a slot leader
  using Threshold = crypto::VRFThreshold;

  /// random value, which serves as a seed for VRF slot leadership selection
  using Randomness = common::Blob<crypto::constants::sr25519::vrf::OUTPUT_SIZE>;

}  // namespace kagome::consensus::babe

#endif  // KAGOME_COMMON_HPP
