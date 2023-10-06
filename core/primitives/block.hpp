/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/block_header.hpp"
#include "primitives/extrinsic.hpp"
#include "scale/tie.hpp"

namespace kagome::primitives {
  using BlockBody = std::vector<Extrinsic>;

  /**
   * @brief Block class represents polkadot block primitive
   */
  struct Block {
    SCALE_TIE(2);

    BlockHeader header;  ///< block header
    BlockBody body{};    ///< extrinsics collection
  };

  struct BlockReflection {
    SCALE_TIE(2);

    BlockHeaderReflection header;  ///< block header
    const BlockBody &body;         ///< extrinsics collection
  };

}  // namespace kagome::primitives
