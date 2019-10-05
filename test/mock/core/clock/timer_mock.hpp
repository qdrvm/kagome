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
    MOCK_METHOD1(expiresAt, void(kagome::clock::SystemClock::TimePoint));

    MOCK_METHOD1(asyncWait,
                 void(const std::function<void(const std::error_code &)> &h));
  };
}  // namespace testutil

#endif  // KAGOME_TIMER_MOCK_HPP
