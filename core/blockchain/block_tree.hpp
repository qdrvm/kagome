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

namespace kagome::blockchain {
  /**
   * Storage for blocks, which has a form of tree
   */
  struct BlockTree {
    using BlockHashRes = outcome::result<primitives::BlockHash>;
    using BlockHashResVec = std::vector<BlockHashRes>;

    virtual ~BlockTree() = default;

    virtual outcome::result<primitives::Block> getBlock(
        const primitives::BlockId &block) const = 0;

    virtual outcome::result<void> addBlock(const primitives::BlockId &parent,
                                           primitives::Block block) = 0;

    virtual BlockHashResVec getChainByBlock(
        const primitives::BlockId &block) const = 0;

    virtual BlockHashResVec longestPath(
        const primitives::BlockId &block) const = 0;

    virtual const primitives::Block &deepestLeaf() const = 0;

    virtual std::vector<primitives::BlockHash> getLeaves() const = 0;

    virtual BlockHashResVec getChildren(
        const primitives::BlockId &block) const = 0;

    virtual BlockHashRes getLastFinalized() const = 0;

    virtual void prune() = 0;
  };
}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCK_TREE_HPP
