/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_TYPES_BLOCK_DIRECTION_HPP
#define KAGOME_CORE_NETWORK_TYPES_BLOCK_DIRECTION_HPP

#include <cstdint>
#include <type_traits>

#include "scale/outcome_throw.hpp"

namespace kagome::network {
  /**
   * Direction, in which to retrieve the blocks
   */
  enum class Direction : uint8_t {
    /// from child to parent
    ASCENDING = 0,
    /// from parent to canonical child
    DESCENDING = 1
  };

  /**
   * @brief outputs object of type Direction to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Direction &v) {
    return s << static_cast<uint8_t>(v);
  }

  /**
   * @brief decodes object of type Direction from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, Direction &v) {
    uint8_t value = 0u;
    s >> value;
    switch (value) {
      case 0:
        v = Direction::ASCENDING;
        break;
      case 1:
        v = Direction::DESCENDING;
        break;
      default:
        scale::common::raise(scale::DecodeError::UNEXPECTED_VALUE);
    }
    v = static_cast<Direction>(value);
    return s;
  }
}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_TYPES_BLOCK_DIRECTION_HPP
