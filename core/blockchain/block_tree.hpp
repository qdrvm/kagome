/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_TREE_HPP
#define KAGOME_BLOCK_TREE_HPP

#include <vector>

#include <outcome/outcome.hpp>
#include "primitives/block.hpp"
#include "primitives/block_id.hpp"
#include "primitives/justification.hpp"

namespace kagome::blockchain {
  /**
   * Storage for blocks, which has a form of tree
   */
  struct BlockTree {
    using BlockHashVecRes = outcome::result<std::vector<primitives::BlockHash>>;

    virtual ~BlockTree() = default;

    virtual outcome::result<primitives::BlockBody> getBlockBody(
        const primitives::BlockId &block) const = 0;

    /**
     * Add a new block to the tree
     * @param block to be added
     * @return nothing or error; if error happens, no changes in the tree are
     * made
     */
    virtual outcome::result<void> addBlock(primitives::Block block) = 0;

    virtual outcome::result<void> finalizeBlock(
        const primitives::BlockHash &block,
        const primitives::Justification &justification) = 0;

    virtual BlockHashVecRes getChainByBlock(
        const primitives::BlockHash &block) = 0;

    virtual BlockHashVecRes longestPath(const primitives::BlockHash &block) = 0;

    virtual const primitives::BlockHash &deepestLeaf() const = 0;

    virtual std::vector<primitives::BlockHash> getLeaves() const = 0;

    virtual BlockHashVecRes getChildren(const primitives::BlockHash &block) = 0;

    virtual primitives::BlockHash getLastFinalized() const = 0;

    virtual void prune() = 0;
  };
}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCK_TREE_HPP
