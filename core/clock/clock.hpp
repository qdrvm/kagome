/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CLOCK_HPP
#define KAGOME_CLOCK_HPP

#include <chrono>

namespace kagome::clock {

  /**
   * An interface for a clock
   * @tparam clock type is an underlying clock type, such as std::steady_clock
   */
  template <typename ClockType>
  class Clock {
   public:
    /**
     * Difference between two time points
     */
    using Duration = typename ClockType::duration;
    /**
     * A moment in time, stored in milliseconds since Unix epoch start
     */
    using TimePoint = typename ClockType::time_point;

    virtual ~Clock() = default;

    /**
     * @return a time point representing the current time
     */
    virtual TimePoint now() const = 0;

    virtual uint64_t nowUint64() const = 0;
  };

  /**
   * SteadyClock alias over Clock. Should be used when we need to measure
   * interval between two moments in time
   */
  using SteadyClock = Clock<std::chrono::steady_clock>;

  /**
   * SystemClock alias over Clock. Should be used when we need to watch current
   * time
   */
  using SystemClock = Clock<std::chrono::system_clock>;

}  // namespace kagome::clock

#endif  // KAGOME_CLOCK_HPP
