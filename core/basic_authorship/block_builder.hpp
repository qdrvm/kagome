/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_BASIC_AUTHORSHIP_BLOCK_BUILDER_HPP
#define KAGOME_CORE_BASIC_AUTHORSHIP_BLOCK_BUILDER_HPP

#include "primitives/block.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::basic_authorship {

  /**
   * BlockBuilder collects extrinsics and creates new block and then should be
   * destroyed
   */
  class BlockBuilder {
   public:
    virtual ~BlockBuilder() = default;

    /**
     * Push extrinsic to wait its inclusion to the block
     * Returns result containing true if xt was pushed, false otherwise and
     * failure if error happened
     */
    virtual outcome::result<bool> pushExtrinsic(
        primitives::Extrinsic extrinsic) = 0;

    /**
     * Create a block from extrinsics and header
     */
    [[nodiscard]] virtual primitives::Block bake() const = 0;
  };

}  // namespace kagome::basic_authorship

#endif  // KAGOME_CORE_BASIC_AUTHORSHIP_BLOCK_BUILDER_HPP
