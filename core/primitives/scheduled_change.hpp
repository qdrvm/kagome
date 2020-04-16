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

#ifndef KAGOME_CORE_PRIMITIVES_SCHEDULED_CHANGE_HPP
#define KAGOME_CORE_PRIMITIVES_SCHEDULED_CHANGE_HPP

#include <boost/optional.hpp>
#include "primitives/authority.hpp"
#include "primitives/common.hpp"
#include "primitives/session_key.hpp"

namespace kagome::primitives {
  /// @struct ScheduledChange is used by Grandpa api runtime
  struct ScheduledChange {
    std::vector<std::pair<AuthorityId, uint64_t>> next_authorities;
    BlockNumber delay = 0;
  };

  /// @brief api function returns optional value
  using ScheduledChangeOptional = boost::optional<ScheduledChange>;

  /// @brief result type for grandpa forced_change function
  using ForcedChange = std::pair<BlockNumber, ScheduledChange>;

  /**
   * @brief outputs ScheduledChange instance to stream
   * @tparam Stream stream type
   * @param s reference to stream
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const ScheduledChange &v) {
    return s << v.next_authorities << v.delay;
  }

  /**
   * @brief decodes ScheduledChange instance from stream
   * @tparam Stream stream type
   * @param s reference to stream
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, ScheduledChange &v) {
    return s >> v.next_authorities >> v.delay;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_SCHEDULED_CHANGE_HPP
