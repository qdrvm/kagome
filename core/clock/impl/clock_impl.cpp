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

#include "clock/impl/clock_impl.hpp"

namespace kagome::clock {

  template <typename ClockType>
  typename Clock<ClockType>::TimePoint ClockImpl<ClockType>::now() const {
    return ClockType::now();
  }

  template <typename ClockType>
  uint64_t ClockImpl<ClockType>::nowUint64() const {
    return std::chrono::duration_cast<std::chrono::seconds>(
               now().time_since_epoch())
        .count();
  }

  template class ClockImpl<std::chrono::steady_clock>;
  template class ClockImpl<std::chrono::system_clock>;

}  // namespace kagome::clock
