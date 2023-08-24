/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_TYPES_HPP
#define KAGOME_CONSENSUS_TYPES_HPP

#include <cstdint>

#include "clock/clock.hpp"

namespace kagome::consensus {

  using Clock = clock::SystemClock;

  /// Consensus uses system clock's time points
  using TimePoint = Clock::TimePoint;

  // Consensus uses system clock's duration
  using Duration = Clock::Duration;

  /// slot number of the block production
  using SlotNumber = uint64_t;

  /// number of the epoch in the block production
  using EpochNumber = uint64_t;

  // number of slots in a single epoch
  using EpochLength = SlotNumber;

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_TYPES_HPP
