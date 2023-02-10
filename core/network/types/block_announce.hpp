/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_ANNOUNCE_HPP
#define KAGOME_BLOCK_ANNOUNCE_HPP

#include "primitives/block_header.hpp"
#include "scale/tie.hpp"

namespace kagome::network {

  /// Block state in the chain.
  enum class BlockState : uint8_t {
    /// Block is not part of the best chain.
    Normal,
    /// Latest best block.
    Best,
  };

  /// Announce a new complete relay chain block on the network.
  struct BlockAnnounce {
    SCALE_TIE(3);

    /// New block header.
    primitives::BlockHeader header;

    /// Block state.
    std::optional<BlockState> state = std::nullopt;

    /// Data associated with this block announcement, e.g. a candidate message.
    std::optional<std::vector<uint8_t>> data = std::nullopt;
  };

}  // namespace kagome::network

#endif  // KAGOME_BLOCK_ANNOUNCE_HPP
