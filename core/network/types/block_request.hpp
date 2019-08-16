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

  /**
   * Direction, in which to retrieve the blocks
   */
  enum class Direction {
    /// from child to parent
    ASCENDING = 0,
    /// from parent to canonical child
    DESCENDING = 1
  };

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
    boost::optional<primitives::BlockHash> to;
    /// sequence direction
    Direction direction;
    /// maximum number of blocks to return; an implementation defined maximum is
    /// used when unspecified
    boost::optional<uint32_t> max;
  };

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
