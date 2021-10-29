/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TIMER_MOCK_HPP
#define KAGOME_TIMER_MOCK_HPP

#include <gmock/gmock.h>
#include "clock/timer.hpp"

namespace testutil {
  struct TimerMock : public kagome::clock::Timer {
    MOCK_METHOD(void,
                expiresAt,
                (kagome::clock::SystemClock::TimePoint),
                (override));

    MOCK_METHOD(void,
                expiresAfter,
                (kagome::clock::SystemClock::Duration),
                (override));

    MOCK_METHOD(void, cancel, (), (override));

    MOCK_METHOD(void,
                asyncWait,
                (const std::function<void(const std::error_code &)> &h));
  };
}  // namespace testutil

#endif  // KAGOME_TIMER_MOCK_HPP
