/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CLOCK_STEADY_CLOCK_HPP
#define KAGOME_CORE_CLOCK_STEADY_CLOCK_HPP

#include "clock/clock.hpp"

#include <chrono>

namespace kagome::clock {

  /**
   * SteadyClock alias over Clock. Should be used when we need to measure
   * interval between two moments in time
   */
  using SteadyClock = Clock<std::chrono::steady_clock>;

}  // namespace kagome::clock

#endif  // KAGOME_CORE_CLOCK_STEADY_CLOCK_HPP
