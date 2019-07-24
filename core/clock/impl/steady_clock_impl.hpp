/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CLOCK_IMPL_STEADY_CLOCK_IMPL_HPP
#define KAGOME_CORE_CLOCK_IMPL_STEADY_CLOCK_IMPL_HPP

#include "clock/steady_clock.hpp"

namespace kagome::clock {

  class SteadyClockImpl : public SteadyClock {
   public:
    ~SteadyClockImpl() override = default;

    TimePoint now() const override;
  };

}  // namespace kagome::clock

#endif  // KAGOME_CORE_CLOCK_IMPL_STEADY_CLOCK_IMPL_HPP
