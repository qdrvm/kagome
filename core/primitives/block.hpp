/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_BLOCK_HPP
#define KAGOME_PRIMITIVES_BLOCK_HPP

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
    SCALE_TIE_EQ(Block);

    BlockHeader header;  ///< block header
    BlockBody body{};    ///< extrinsics collection
  };
}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_BLOCK_HPP
