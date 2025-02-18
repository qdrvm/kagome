/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/block_header.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::primitives {
  using BlockBody = std::vector<Extrinsic>;

  /**
   * @brief Block class represents polkadot block primitive
   */
  struct Block {
    BlockHeader header;  ///< block header
    BlockBody body{};    ///< extrinsics collection
    bool operator==(const Block &other) const = default;
  };

  struct BlockReflection {
    BlockHeaderReflection header;  ///< block header
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    const BlockBody &body;  ///< extrinsics collection
    SCALE_CUSTOM_DECOMPOSITION(BlockReflection, header, body)
  };

}  // namespace kagome::primitives
