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
   * The BlockBuilderFactory class is responsible for creating new block
   * builders. Each block builder encapsulates the logic for creating a single
   * block from provided block information. This class is used in the block
   * production process, specifically in the propose method of the ProposerImpl
   * class.
   */
  class BlockBuilderFactory {
   public:
    virtual ~BlockBuilderFactory() = default;

    /**
     * The make method prepares a BlockBuilder for creating a block on top of a
     * parent block and using provided digests. It also initializes the block
     * created in BlockBuilder. This method is called in the propose method of
     * the ProposerImpl class.
     *
     * @param parent_block The block that the new block will be built on top of.
     * @param inherent_digest The digest that will be used in the creation of
     * the new block.
     * @param changes_tracker Tracks changes to the trie during the block
     * production process.
     * @return A unique pointer to a BlockBuilder, or an error if the
     * BlockBuilder could not be created.
     */
    virtual outcome::result<std::unique_ptr<BlockBuilder>> make(
        const primitives::BlockInfo &parent_block,
        primitives::Digest inherent_digest,
        TrieChangesTrackerOpt changes_tracker) const = 0;
  };

}  // namespace kagome::authorship
