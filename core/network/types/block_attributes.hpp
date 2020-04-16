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

#ifndef KAGOME_CORE_NETWORK_TYPES_BLOCK_ATTRIBUTES_HPP
#define KAGOME_CORE_NETWORK_TYPES_BLOCK_ATTRIBUTES_HPP

#include <bitset>
#include <cstdint>

#include "common/outcome_throw.hpp"
#include "scale/scale_error.hpp"

namespace kagome::network {

  /**
   * Masks of bits, combination of which shows, which fields are to be presented
   * in the BlockResponse
   */
  enum class BlockAttributesBits : uint8_t {
    /// Include block header.
    HEADER = 1u,
    /// Include block body.
    BODY = 1u << 1u,
    /// Include block receipt.
    RECEIPT = 1u << 2u,
    /// Include block message queue.
    MESSAGE_QUEUE = 1u << 3u,
    /// Include a justification for the block.
    JUSTIFICATION = 1u << 4u
  };

  /**
   * Block attributes as set of bits
   */
  struct BlockAttributes {
    std::bitset<8> attributes;
  };

  /**
   * @brief compares two BlockAttributes instances
   * @param lhs first BlockAttributes instance
   * @param rhs second BlockAttributes instance
   * @return true if equal false otherwise
   */
  inline bool operator==(const BlockAttributes &lhs,
                         const BlockAttributes &rhs) {
    return lhs.attributes == rhs.attributes;
  }

  /**
   * @brief combines block attributes bits
   * @param lhs first BlockAttributesBits value
   * @param rhs second BlockAttributesBits value
   * @return logical `or` operation applied to the values as uint8_t
   */
  inline uint8_t operator|(BlockAttributesBits lhs, BlockAttributesBits rhs) {
    return static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs);
  }

  /**
   * @brief combines block attributes bits
   * @param lhs first BlockAttributesBits value as uint8_t
   * @param rhs second BlockAttributesBits value
   * @return logical `or` operation applied to the values as uint8_t
   */
  inline uint8_t operator|(uint8_t lhs, BlockAttributesBits rhs) {
    return lhs | static_cast<uint8_t>(rhs);
  }

  /**
   * @brief combines block attributes bits
   * @param lhs first BlockAttributesBits value
   * @param rhs second BlockAttributesBits value as uint8_t
   * @return logical `or` operation applied to the values as uint8_t
   */
  inline uint8_t operator|(BlockAttributesBits lhs, uint8_t rhs) {
    return static_cast<uint8_t>(lhs) | rhs;
  }

  /**
   * @brief outputs object of type BlockAttributes to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const BlockAttributes &v) {
    return s << static_cast<uint8_t>(v.attributes.to_ulong());
  }

  /**
   * decodes object of type BlockAttributes from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, BlockAttributes &v) {
    uint8_t value = 0u;
    s >> value;
    constexpr uint8_t unused_bits = 0b11100000;
    if ((value & unused_bits) != 0u) {
      common::raise(scale::DecodeError::UNEXPECTED_VALUE);
    }
    v = BlockAttributes{value};
    return s;
  }

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_TYPES_BLOCK_ATTRIBUTES_HPP
