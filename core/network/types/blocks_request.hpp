/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKS_REQUEST_HPP
#define KAGOME_BLOCKS_REQUEST_HPP

#include <cstdint>

#include <boost/optional.hpp>
#include <gsl/span>
#include "network/types/block_attributes.hpp"
#include "network/types/block_direction.hpp"
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  /**
   * Request for blocks to another peer
   */
  struct BlocksRequest {
    /// unique request id
    primitives::BlocksRequestId id;
    /// bits, showing, which parts of BlockData to return
    BlockAttributes fields{};
    /// start from this block
    primitives::BlockId from{};
    /// end at this block; an implementation defined maximum is used when
    /// unspecified
    boost::optional<primitives::BlockHash> to{};
    /// sequence direction
    Direction direction{};
    /// maximum number of blocks to return; an implementation defined maximum is
    /// used when unspecified
    boost::optional<uint32_t> max{};

    /// includes HEADER, BODY and JUSTIFICATION
    static constexpr BlockAttributes kBasicAttributes{19};

    bool attributeIsSet(BlockAttributesBits attribute) const {
      return (fields.attributes.to_ulong() & static_cast<uint8_t>(attribute))
             != 0;
    }
  };
}  // namespace kagome::network

#endif  // KAGOME_BLOCKS_REQUEST_HPP
