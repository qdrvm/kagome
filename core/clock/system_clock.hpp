/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CLOCK_SYSTEM_CLOCK_HPP
#define KAGOME_CORE_CLOCK_SYSTEM_CLOCK_HPP

#include "clock/clock.hpp"

#include <chrono>

namespace kagome::clock {

  /**
   * SystemClock alias over Clock. Should be used when we need to watch current
   * time
   */
  using SystemClock = Clock<std::chrono::system_clock>;

}  // namespace kagome::clock

#endif  // KAGOME_CORE_CLOCK_SYSTEM_CLOCK_HPP
