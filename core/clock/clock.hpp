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

    /**
     * @return uint64_t representing number of seconds since the beginning of
     * epoch (Jan 1, 1970)
     */
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
