/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/impl/steady_clock_impl.hpp"

namespace kagome::clock {

  SteadyClock::TimePoint SteadyClockImpl::now() const {
    return std::chrono::steady_clock::now();
  }
}  // namespace kagome::clock
