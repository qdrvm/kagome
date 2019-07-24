/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SYSTEM_CLOCK_HPP
#define KAGOME_SYSTEM_CLOCK_HPP

#include "clock/system_clock.hpp"

namespace kagome::clock {

  class SystemClockImpl : public SystemClock {
   public:
    ~SystemClockImpl() override = default;

    TimePoint now() const override;
  };

}  // namespace kagome::common

#endif  // KAGOME_SYSTEM_CLOCK_HPP
