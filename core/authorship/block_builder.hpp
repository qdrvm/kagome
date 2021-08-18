/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_HPP
#define KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_HPP

#include "primitives/block.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/inherent_data.hpp"

namespace kagome::authorship {

  /**
   * BlockBuilder collects extrinsics and creates new block and then should be
   * destroyed
   */
  class BlockBuilder {
   public:
    virtual ~BlockBuilder() = default;

    virtual outcome::result<std::vector<primitives::Extrinsic>>
    getInherentExtrinsics(const primitives::InherentData &data) const = 0;

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

    /**
     * Estimate size of encoded block representation
     * @return size in bytes
     */
    virtual size_t estimateBlockSize() const = 0;
  };

}  // namespace kagome::authorship

#endif  // KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_HPP
