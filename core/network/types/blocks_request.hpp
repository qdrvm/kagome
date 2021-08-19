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

  /**
   * @brief compares two BlockRequest instances
   * @param lhs first instance
   * @param rhs second instance
   * @return true if equal false otherwise
   */
  inline bool operator==(const BlocksRequest &lhs, const BlocksRequest &rhs) {
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
  Stream &operator<<(Stream &s, const BlocksRequest &v) {
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
  Stream &operator>>(Stream &s, BlocksRequest &v) {
    return s >> v.id >> v.fields >> v.from >> v.to >> v.direction >> v.max;
  }
}  // namespace kagome::network

#endif  // KAGOME_BLOCKS_REQUEST_HPP
