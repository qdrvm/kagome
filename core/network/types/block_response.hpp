/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_RESPONSE_HPP
#define KAGOME_BLOCK_RESPONSE_HPP

#include <vector>

#include <boost/optional.hpp>
#include "common/buffer.hpp"
#include "primitives/block.hpp"
#include "primitives/common.hpp"
#include "primitives/justification.hpp"

namespace kagome::network {
  /**
   * Data, describing the requested block
   */
  struct BlockData {
    primitives::BlockHash hash;
    boost::optional<primitives::BlockHeader> header;
    boost::optional<primitives::BlockBody> body;
    boost::optional<common::Buffer> receipt;
    boost::optional<common::Buffer> message_queue;
    boost::optional<primitives::Justification> justification;
  };

  /**
   * Response to the BlockRequest
   */
  struct BlockResponse {
    primitives::BlockRequestId id;
    std::vector<BlockData> blocks;
  };
}  // namespace kagome::network

#endif  // KAGOME_BLOCK_RESPONSE_HPP
