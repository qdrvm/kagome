/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TICKER_MOCK_HPP
#define KAGOME_TICKER_MOCK_HPP

#include <gmock/gmock.h>
#include "clock/ticker.hpp"

namespace testutil {
  struct TickerMock : public kagome::clock::Ticker {
    MOCK_METHOD1(start, void(kagome::clock::SystemClock::Duration));

    MOCK_METHOD0(stop, void());

    MOCK_METHOD0(isStarted, bool());

    MOCK_METHOD0(interval, kagome::clock::SystemClock::Duration());

    MOCK_METHOD1(asyncCallRepeatedly,
                 void(std::function<void(const std::error_code &)> h));
  };
}  // namespace testutil

#endif  // KAGOME_TICKER_MOCK_HPP
