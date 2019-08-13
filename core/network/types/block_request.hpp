/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_REQUEST_HPP
#define KAGOME_BLOCK_REQUEST_HPP

#include <cstdint>

#include <boost/optional.hpp>
#include <gsl/span>
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  using BlockAttributes = uint8_t;
  /**
   * Masks of bits, combination of which shows, which fields are to be presented
   * in the BlockResponse
   */
  enum class BlockAttributesBits : BlockAttributes {
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
   * Direction, in which to retrieve the blocks
   */
  enum class Direction {
    /// from child to parent
    ASCENDING = 0,
    /// from parent to canonical child
    DESCENDING = 1
  };

  // TODO(akvinikym) PRE-279: add codec for this type
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

    /// includes HEADER, BODY and JUSTIFICATION
    static constexpr BlockAttributes kBasicAttributes{19};

    constexpr bool attributeIsSet(BlockAttributesBits attribute) const {
      return fields & static_cast<uint8_t>(attribute);
    }
  };
}  // namespace kagome::network

#endif  // KAGOME_BLOCK_REQUEST_HPP
