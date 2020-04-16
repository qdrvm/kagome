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
