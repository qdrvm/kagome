/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_CLOCK_CLOCK_MOCK_HPP
#define KAGOME_TEST_CORE_CLOCK_CLOCK_MOCK_HPP

#include "clock/clock.hpp"

#include <gmock/gmock.h>

namespace kagome::clock {

  class SystemClockMock : public SystemClock {
   public:
    MOCK_METHOD(SystemClock::TimePoint, now, (), (const, override));

    MOCK_METHOD(uint64_t, nowUint64, (), (const, override));
  };

  class SteadyClockMock : public SteadyClock {
   public:
    MOCK_METHOD(SteadyClock::TimePoint, now, (), (const, override));

    MOCK_METHOD(uint64_t, nowUint64, (), (const, override));
  };

}  // namespace kagome::clock

#endif  // KAGOME_TEST_CORE_CLOCK_CLOCK_MOCK_HPP
