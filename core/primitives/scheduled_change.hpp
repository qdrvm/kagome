/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_SCHEDULED_CHANGE_HPP
#define KAGOME_CORE_PRIMITIVES_SCHEDULED_CHANGE_HPP

#include <boost/optional.hpp>
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
