/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKS_RESPONSE_HPP
#define KAGOME_BLOCKS_RESPONSE_HPP

#include <vector>

#include <optional>
#include "common/buffer.hpp"
#include "primitives/block.hpp"
#include "primitives/block_data.hpp"
#include "primitives/common.hpp"
#include "primitives/justification.hpp"

namespace kagome::network {

  /**
   * Response to the BlockRequest
   */
  struct BlocksResponse {
    std::vector<primitives::BlockData> blocks{};
  };

}  // namespace kagome::network

#endif  // KAGOME_BLOCKS_RESPONSE_HPP
