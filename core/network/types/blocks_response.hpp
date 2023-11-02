/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/size_limited_containers.hpp"
#include "primitives/block_data.hpp"

namespace kagome::network {

  constexpr size_t kMaxBlocksInResponse = 256;

  /**
   * Response to the BlockRequest
   */
  struct BlocksResponse {
    common::SLVector<primitives::BlockData, kMaxBlocksInResponse> blocks{};
    bool multiple_justifications = false;
  };

}  // namespace kagome::network
