/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_REQUEST_HPP
#define KAGOME_BLOCK_REQUEST_HPP

#include <cstdint>

#include <boost/optional.hpp>
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  /**
   * Masks of bits, combination of which shows, which fields are to be presented
   * in the BlockResponse
   */
  enum class BlockAttributesBits : uint8_t {
    /// Include block header.
    HEADER = 0b00000001,
    /// Include block body.
    BODY = 0b00000010,
    /// Include block receipt.
    RECEIPT = 0b00000100,
    /// Include block message queue.
    MESSAGE_QUEUE = 0b00001000,
    /// Include a justification for the block.
    JUSTIFICATION = 0b00010000
  };
  using BlockAttributes = uint8_t;

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
}  // namespace kagome::network

#endif  // KAGOME_BLOCK_REQUEST_HPP
