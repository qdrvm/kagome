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

#ifndef KAGOME_TEST_CORE_CLOCK_CLOCK_MOCK_HPP
#define KAGOME_TEST_CORE_CLOCK_CLOCK_MOCK_HPP

#include "clock/clock.hpp"

#include <gmock/gmock.h>

namespace kagome::clock {

  class SystemClockMock : public SystemClock {
   public:
    MOCK_CONST_METHOD0(now, SystemClock::TimePoint());
    MOCK_CONST_METHOD0(nowUint64, uint64_t());
  };

  class SteadyClockMock : public SteadyClock {
   public:
    MOCK_CONST_METHOD0(now, SteadyClock::TimePoint());
    MOCK_CONST_METHOD0(nowUint64, uint64_t());
  };

}  // namespace kagome::clock

#endif  // KAGOME_TEST_CORE_CLOCK_CLOCK_MOCK_HPP
