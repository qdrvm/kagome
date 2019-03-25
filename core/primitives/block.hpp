/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_BLOCK_HPP
#define KAGOME_PRIMITIVES_BLOCK_HPP

#include "common/buffer.hpp"
#include "primitives/block_header.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::primitives {
  /**
   * @brief Block class represents block
   */
  class Block {
   public:
    /**
     * @brief Block constructor
     * @param header block header
     * @param extrinsics collection of extrinsics
     */
    Block(BlockHeader header, std::vector<Extrinsic> extrinsics);

    /**
     * @return block header const reference
     */
    const BlockHeader &header() const;

    /**
     * @return const reference to extrinsics collection
     */
    const std::vector<Extrinsic> &extrinsics() const;

   private:
    BlockHeader header_;                 ///< block header
    std::vector<Extrinsic> extrinsics_;  ///< extrinsics collection
  };

}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_BLOCK_HPP
