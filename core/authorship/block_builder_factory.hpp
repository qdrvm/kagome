/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "authorship/block_builder.hpp"
#include "primitives/common.hpp"
#include "runtime/runtime_api/block_builder.hpp"
#include "runtime/runtime_api/core.hpp"

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
    virtual outcome::result<std::unique_ptr<BlockBuilder>> make(
        const primitives::BlockInfo &parent_block,
        primitives::Digest inherent_digest,
        TrieChangesTrackerOpt changes_tracker) const = 0;
  };

}  // namespace kagome::authorship
