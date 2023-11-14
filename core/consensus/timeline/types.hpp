/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

#include "clock/clock.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/authority.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus {

  using Clock = clock::SystemClock;

  /// Consensus uses system clock's time points
  using TimePoint = Clock::TimePoint;

  // Consensus uses system clock's duration
  using Duration = Clock::Duration;

  /// slot number of the block production
  using SlotNumber = uint64_t;

  /// duration of single slot in milliseconds
  struct SlotDuration {
    SlotDuration() = default;

    template <typename Rep>
    SlotDuration(const std::chrono::duration<Rep> &duration)
        : value(std::chrono::duration_cast<std::chrono::milliseconds>(duration)
                    .count()) {}

    template <class Rep, std::enable_if<std::is_integral_v<Rep>>>
    SlotDuration(Rep duration) : value(duration) {}

    template <class Rep, typename = std::enable_if<std::is_integral_v<Rep>>>
    operator Rep() const {
      return value;
    }

    operator bool() const {
      return value != 0;
    }

    operator std::chrono::milliseconds() const {
      return std::chrono::milliseconds(value);
    }

    friend ::scale::ScaleEncoderStream &operator<<(
        ::scale::ScaleEncoderStream &s, const SlotDuration &duration) {
      return s << duration.value;
    }

    friend ::scale::ScaleDecoderStream &operator>>(
        ::scale::ScaleDecoderStream &s, SlotDuration &duration) {
      return s >> duration.value;
    }

   private:
    uint64_t value = 0;
  };

  /// number of the epoch in the block production
  using EpochNumber = uint64_t;

  // number of slots in a single epoch
  using EpochLength = SlotNumber;

  /// threshold, which must not be exceeded for the party to be a slot leader
  using Threshold = crypto::VRFThreshold;

  /// random value, which serves as a seed for VRF slot leadership selection
  using Randomness = common::Blob<crypto::constants::sr25519::vrf::OUTPUT_SIZE>;

  /// Data are corresponding to the epoch
  struct EpochDigest {
    SCALE_TIE(2);

    /// The authorities actual for corresponding epoch
    primitives::AuthorityList authorities;

    /// The value of randomness to use for the slot-assignment.
    Randomness randomness;
  };

}  // namespace kagome::consensus
