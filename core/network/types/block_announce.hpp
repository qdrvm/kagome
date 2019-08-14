/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_ANNOUNCE_HPP
#define KAGOME_BLOCK_ANNOUNCE_HPP

#include "primitives/block_header.hpp"

namespace kagome::network {
  // TODO(akvinikym) PRE-279: add codec for this type
  /**
   * Announce a new complete block on the network
   */
  struct BlockAnnounce {
    primitives::BlockHeader header;

    bool operator==(const BlockAnnounce &other) const {
      return header == other.header;
    }
    bool operator!=(const BlockAnnounce &other) const {
      return !(*this == other);
    }
  };
}  // namespace kagome::network

#endif  // KAGOME_BLOCK_ANNOUNCE_HPP
