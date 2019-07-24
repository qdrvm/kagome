/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/impl/system_clock_impl.hpp"

namespace kagome::clock {

  SystemClock::TimePoint SystemClockImpl::now() const {
    return std::chrono::system_clock::now();
  }

}  // namespace kagome::clock
