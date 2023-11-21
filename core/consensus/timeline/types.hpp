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
  struct SlotDuration : public std::chrono::milliseconds {
    SlotDuration() = default;

    template <typename Rep, typename Period>
    SlotDuration(const std::chrono::duration<Rep, Period> &duration)
        : std::chrono::milliseconds(
            std::chrono::duration_cast<std::chrono::milliseconds>(duration)) {}

    template <class Rep>
      requires std::is_integral_v<Rep>
    SlotDuration(Rep duration) : std::chrono::milliseconds(duration) {}

    // Convert to integer
    template <class Rep, typename = std::enable_if<std::is_integral_v<Rep>>>
    operator Rep() const {
      return count();
    }

    // Convert to boolean
    operator bool() const {
      return count() != 0;
    }

    // Convert to duration
    template <typename Rep, typename Period>
    operator std::chrono::duration<Rep, Period>() const {
      return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(
          *this);
    }

    auto operator<=>(const SlotDuration &r) const {
      return count() <=> r.count();
    }

    friend ::scale::ScaleEncoderStream &operator<<(
        ::scale::ScaleEncoderStream &s, const SlotDuration &duration) {
      return s << duration.count();
    }

    friend ::scale::ScaleDecoderStream &operator>>(
        ::scale::ScaleDecoderStream &s, SlotDuration &duration) {
      uint64_t v;
      s >> v;
      duration = {v};
      return s;
    }
  };

  /// number of the epoch in the block production
  using EpochNumber = uint64_t;

  // number of slots in a single epoch
  using EpochLength = SlotNumber;

  /// threshold, which must not be exceeded for the party to be a slot leader
  using Threshold = crypto::VRFThreshold;

  /// random value, which serves as a seed for VRF slot leadership selection
  using Randomness = common::Blob<crypto::constants::sr25519::vrf::OUTPUT_SIZE>;

  struct EpochTimings {
    /// Duration of a slot in milliseconds
    SlotDuration slot_duration{0};

    /// Epoch length in slots
    EpochLength epoch_length{0};

    operator bool() const {
      return (bool)slot_duration and (bool) epoch_length;
    }

    void init(SlotDuration _slot_duration, EpochLength _epoch_length) {
      BOOST_ASSERT_MSG(not *this, "Epoch timings are already initialized");
      slot_duration = _slot_duration;
      epoch_length = _epoch_length;
      BOOST_ASSERT_MSG(*this, "Epoch timings must not be zero");
    }
  };

}  // namespace kagome::consensus
