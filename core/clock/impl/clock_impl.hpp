/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CLOCK_IMPL_CLOCK_IMPL_HPP
#define KAGOME_CORE_CLOCK_IMPL_CLOCK_IMPL_HPP

#include "clock/clock.hpp"

namespace kagome::clock {

  template <typename ClockType>
  class ClockImpl : public Clock<ClockType> {
   public:
    typename Clock<ClockType>::TimePoint now() const override;
    uint64_t nowUint64() const override;
  };

  // aliases for implementations
  using SteadyClockImpl = ClockImpl<std::chrono::steady_clock>;
  using SystemClockImpl = ClockImpl<std::chrono::system_clock>;

}  // namespace kagome::clock

#endif  // KAGOME_CORE_CLOCK_IMPL_CLOCK_IMPL_HPP
