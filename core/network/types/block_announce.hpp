/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_ANNOUNCE_HPP
#define KAGOME_BLOCK_ANNOUNCE_HPP

#include "primitives/block_header.hpp"
#include "scale/tie.hpp"

namespace kagome::network {
  /**
   * Announce a new complete block on the network
   */
  struct BlockAnnounce {
    SCALE_TIE(1);

    primitives::BlockHeader header;
  };
}  // namespace kagome::network

#endif  // KAGOME_BLOCK_ANNOUNCE_HPP
