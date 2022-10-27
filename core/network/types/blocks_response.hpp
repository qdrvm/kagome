/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKS_RESPONSE_HPP
#define KAGOME_BLOCKS_RESPONSE_HPP

#include "common/size_limited_containers.hpp"
#include "primitives/block_data.hpp"

namespace kagome::network {

  constexpr size_t kMaxBlocksInResponse = 256;

  /**
   * Response to the BlockRequest
   */
  struct BlocksResponse {
    common::SLVector<primitives::BlockData, kMaxBlocksInResponse> blocks{};
  };

}  // namespace kagome::network

#endif  // KAGOME_BLOCKS_RESPONSE_HPP
