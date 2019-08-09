/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_FACTORY_HPP
#define KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_FACTORY_HPP

#include <vector>

#include "authorship/block_builder.hpp"
#include "primitives/block_id.hpp"
#include "runtime/block_builder_api.hpp"
#include "runtime/core.hpp"

namespace kagome::authorship {

  /**
   * Creates new block builders. Each of them encapsulates the logic for
   * creating a single block from provided block information
   */
  class BlockBuilderFactory {
   public:
    virtual ~BlockBuilderFactory() = default;

    /**
     * Prepares BlockBuilder for creating block on top of parent block and using
     * provided digests. Also initialises the block created in BlockBuilder
     */
    virtual outcome::result<std::unique_ptr<BlockBuilder>> create(
        const primitives::BlockId &parent_id,
        std::vector<primitives::Digest> inherent_digests) const = 0;
  };

}  // namespace kagome::authorship

#endif  // KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_FACTORY_HPP
