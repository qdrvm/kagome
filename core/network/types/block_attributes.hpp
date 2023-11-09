/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <bitset>
#include <cstdint>

#include "common/outcome_throw.hpp"
#include "scale/scale_error.hpp"

#define BLOCK_ATTRIBUTE_OP(op)                                           \
  constexpr auto operator op(BlockAttributes l, BlockAttributes r) {     \
    return static_cast<BlockAttributes>(static_cast<uint8_t>(l)          \
                                            op static_cast<uint8_t>(r)); \
  }

namespace kagome::network {

  /**
   * Masks of bits, combination of which shows, which fields are to be presented
   * in the BlockResponse
   */
  enum class BlockAttributes : uint8_t {
    /// Include block header.
    HEADER = 1u,
    /// Include block body.
    BODY = 1u << 1u,
    /// Include block receipt.
    RECEIPT = 1u << 2u,
    /// Include block message queue.
    MESSAGE_QUEUE = 1u << 3u,
    /// Include a justification for the block.
    JUSTIFICATION = 1u << 4u,
    _MASK = 0b11111,
  };
  BLOCK_ATTRIBUTE_OP(&)
  BLOCK_ATTRIBUTE_OP(|)
  inline BlockAttributes toBlockAttributes(uint8_t v) {
    return BlockAttributes{v} & BlockAttributes::_MASK;
  }
  inline bool has(BlockAttributes l, BlockAttributes r) {
    return (l & r) == r;
  }
  using BlockAttribute = BlockAttributes;

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
    return s << static_cast<uint8_t>(v);
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
  Stream &operator>>(Stream &s, BlockAttributes &attributes) {
    uint8_t value = 0u;
    s >> value;
    attributes = toBlockAttributes(value);
    if (static_cast<uint8_t>(attributes) != value) {
      common::raise(scale::DecodeError::UNEXPECTED_VALUE);
    }
    return s;
  }

}  // namespace kagome::network

template <>
struct std::hash<kagome::network::BlockAttributes> {
  auto operator()(const kagome::network::BlockAttributes &attr) const {
    return std::hash<uint8_t>()(static_cast<uint8_t>(attr));
  }
};

#undef BLOCK_ATTRIBUTE_OP
