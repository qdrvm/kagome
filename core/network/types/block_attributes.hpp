/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <scale/scale.hpp>

#include "common/outcome_throw.hpp"

#include <scale/kagome_scale.hpp>

#define BLOCK_ATTRIBUTE_OP(op)                                          \
  constexpr auto operator op(BlockAttribute l, BlockAttribute r) {      \
    return static_cast<BlockAttribute>(static_cast<uint8_t>(l)          \
                                           op static_cast<uint8_t>(r)); \
  }

namespace kagome::network {

  /**
   * Masks of bits, combination of which shows, which fields are to be presented
   * in the BlockResponse
   */
  enum class BlockAttribute : uint8_t {
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

  inline BlockAttribute toBlockAttribute(uint8_t v) {
    return BlockAttribute{v} & BlockAttribute::_MASK;
  }

  inline bool has(BlockAttribute l, BlockAttribute r) {
    return (l & r) == r;
  }
}  // namespace kagome::network

namespace scale {
  inline void encode(kagome::network::BlockAttribute &attributes,
                     scale::Encoder &encoder) {
    encoder.put(static_cast<uint8_t>(attributes));
  }

  inline void decode(kagome::network::BlockAttribute &attributes,
                     scale::Decoder &decoder) {
    uint8_t value = decoder.take();
    attributes = kagome::network::toBlockAttribute(value);
    if (static_cast<uint8_t>(attributes) != value) {
      kagome::scale::raise(scale::DecodeError::UNEXPECTED_VALUE);
    }
  }

}  // namespace scale

template <>
struct std::hash<kagome::network::BlockAttribute> {
  auto operator()(const kagome::network::BlockAttribute &attr) const {
    return std::hash<uint8_t>()(static_cast<uint8_t>(attr));
  }
};

#undef BLOCK_ATTRIBUTE_OP
