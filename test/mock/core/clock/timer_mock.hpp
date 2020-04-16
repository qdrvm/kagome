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
