/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_REQUEST_HPP
#define KAGOME_BLOCK_REQUEST_HPP

#include <bitset>
#include <cstdint>

#include <boost/optional.hpp>
#include <gsl/span>
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"

using namespace kagome::common;

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
  using BlockAttributes = std::bitset<8>;

  inline uint8_t operator|(BlockAttributesBits lhs, BlockAttributesBits rhs) {
    return static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs);
  }

  inline uint8_t operator|(uint8_t lhs, BlockAttributesBits rhs) {
    return lhs | static_cast<uint8_t>(rhs);
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
    return s << static_cast<uint8_t>(v.to_ulong());
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
    v = value;
    return s;
  }

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
    v = static_cast<Direction>(value);
    return s;
  }

  /**
   * Request for blocks to another peer
   */
  struct BlockRequest {
    /// unique request id
    primitives::BlockRequestId id;
    /// bits, showing, which parts of BlockData to return
    BlockAttributes fields;
    /// start from this block
    primitives::BlockId from;
    /// end at this block; an implementation defined maximum is used when
    /// unspecified
    std::optional<primitives::BlockHash> to;
    /// sequence direction
    Direction direction;
    /// maximum number of blocks to return; an implementation defined maximum is
    /// used when unspecified
    std::optional<uint32_t> max;
  };

  /**
   * @brief compares two BlockRequest instances
   * @param lhs first instance
   * @param rhs second instance
   * @return true if equal false otherwise
   */
  inline bool operator==(const BlockRequest &lhs, const BlockRequest &rhs) {
    return lhs.id == rhs.id && lhs.fields == rhs.fields && lhs.from == rhs.from
           && lhs.to == rhs.to && lhs.direction == rhs.direction
           && lhs.max == rhs.max;
  }

  /**
   * @brief outputs object of type BlockRequest to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const BlockRequest &v) {
    return s << v.id << v.fields << v.from << v.to << v.direction << v.max;
  }

  /**
   * @brief decodes object of type BlockRequest from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, BlockRequest &v) {
    return s >> v.id >> v.fields >> v.from >> v.to >> v.direction >> v.max;
  }
}  // namespace kagome::network

#endif  // KAGOME_BLOCK_REQUEST_HPP
