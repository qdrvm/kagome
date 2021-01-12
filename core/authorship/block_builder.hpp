/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_HPP
#define KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_HPP

#include "primitives/block.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::authorship {

  /**
   * BlockBuilder collects extrinsics and creates new block and then should be
   * destroyed
   */
  class BlockBuilder {
   public:
    virtual ~BlockBuilder() = default;

    /**
     * Push extrinsic to wait its inclusion to the block
     * Returns result containing success if xt was pushed, error otherwise
     */
    virtual outcome::result<primitives::ExtrinsicIndex> pushExtrinsic(
        const primitives::Extrinsic &extrinsic) = 0;

    /**
     * Create a block from extrinsics and header
     */
    virtual outcome::result<primitives::Block> bake() const = 0;
  };

}  // namespace kagome::authorship

#endif  // KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_HPP
