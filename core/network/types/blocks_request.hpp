/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKS_REQUEST_HPP
#define KAGOME_BLOCKS_REQUEST_HPP

#include <cstdint>

#include <gsl/span>
#include <optional>

#include "network/types/block_attributes.hpp"
#include "network/types/block_direction.hpp"
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  /**
   * Request for blocks to another peer
   */
  struct BlocksRequest {
    /// bits, showing, which parts of BlockData to return
    BlockAttributes fields{};
    /// start from this block
    primitives::BlockId from{};
    /// end at this block; an implementation defined maximum is used when
    /// unspecified
    std::optional<primitives::BlockHash> to{};
    /// sequence direction
    Direction direction{};
    /// maximum number of blocks to return; an implementation defined maximum is
    /// used when unspecified
    std::optional<uint32_t> max{};

    /// includes HEADER, BODY and JUSTIFICATION
    static constexpr BlockAttributes kBasicAttributes =
        BlockAttribute::HEADER | BlockAttribute::BODY
        | BlockAttribute::JUSTIFICATION;

    bool attributeIsSet(const BlockAttribute &attribute) const {
      return fields & attribute;
    }

    inline size_t fingerprint() const;
  };
}  // namespace kagome::network

template <>
struct std::hash<kagome::network::BlocksRequest> {
  auto operator()(const kagome::network::BlocksRequest &blocks_request) const {
    auto result =
        std::hash<kagome::network::BlockAttributes>()(blocks_request.fields);

    boost::hash_combine(
        result, std::hash<kagome::primitives::BlockId>()(blocks_request.from));

    boost::hash_combine(
        result,
        std::hash<std::optional<kagome::primitives::BlockHash>>()(
            blocks_request.to));

    boost::hash_combine(
        result,
        std::hash<kagome::network::Direction>()(blocks_request.direction));

    boost::hash_combine(
        result, std::hash<std::optional<uint32_t>>()(blocks_request.max));

    return result;
  }
};

inline size_t kagome::network::BlocksRequest::fingerprint() const {
  return std::hash<BlocksRequest>()(*this);
}

#endif  // KAGOME_BLOCKS_REQUEST_HPP
