/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_TREE_HPP
#define KAGOME_BLOCK_TREE_HPP

#include <cstdint>
#include <vector>

#include <outcome/outcome.hpp>
#include "primitives/block.hpp"
#include "primitives/block_id.hpp"
#include "primitives/justification.hpp"

namespace kagome::blockchain {
  /**
   * Storage for blocks, which has a form of tree; it serves two functions:
   *    - keep tracking of all finalized blocks (they are kept in the
   *      non-volatile storage)
   *    - work with blocks, which participate in the current round of BABE block
   *      production (handling forks, pruning the blocks, resolving child-parent
   *      relations, etc)
   */
  struct BlockTree {
    using BlockHashVecRes = outcome::result<std::vector<primitives::BlockHash>>;

    virtual ~BlockTree() = default;

    /**
     * Get a body (extrinsics) of the block (if present)
     * @param block - id of the block to get body for
     * @return body, if the block exists in our storage, error in case it does
     * not exist in our storage, or actual error happens
     */
    virtual outcome::result<primitives::BlockBody> getBlockBody(
        const primitives::BlockId &block) const = 0;

    /**
     * Get a justification of the block (if present)
     * @param block - id of the block to get justification for
     * @return body, if the block exists in our storage, error in case it does
     * not exist in our storage, or actual error happens
     */
    virtual outcome::result<primitives::Justification> getBlockJustification(
        const primitives::BlockId &block) const = 0;

    /**
     * Add a new block to the tree
     * @param block to be added
     * @return nothing or error; if error happens, no changes in the tree are
     * made
     *
     * @note if block, which is specified in PARENT_HASH field of (\param block)
     * is not in our local storage, corresponding error is returned. It is
     * suggested that after getting that error, the caller would ask another
     * peer for the parent block and try to insert it; this operation is to be
     * repeated until a successful insertion happens
     */
    virtual outcome::result<void> addBlock(primitives::Block block) = 0;

    /**
     * Mark the block as finalized and store a finalization justification
     * @param block to be finalized
     * @param justification of the finalization
     * @return nothing or error
     */
    virtual outcome::result<void> finalize(
        const primitives::BlockHash &block,
        const primitives::Justification &justification) = 0;

    /**
     * Get a chain of blocks from the specified as a (\param block) up to the
     * closest finalized one
     * @param block to get a chain from
     * @return chain of blocks in top-to-bottom order (from the last finalized
     * block to the provided one) or error
     */
    virtual BlockHashVecRes getChainByBlock(
        const primitives::BlockHash &block) = 0;

    /**
     * Get a chain of blocks from the (\param block)
     * @param block, from which the chain is started
     * @param ascending - if true, the chain will grow up from the provided
     * block (it is the lowest one); if false, down
     * @param maximum number of blocks to be retrieved
     * @return chain or blocks or error
     */
    virtual BlockHashVecRes getChainByBlock(const primitives::BlockHash &block,
                                            bool ascending,
                                            uint64_t maximum) = 0;

    /**
     * Get a chain of blocks
     * @param top_block - block, which is at the top of the chain
     * @param bottom_block - block, which is the bottim of the chain
     * @return chain of blocks in top-to-bottom order or error
     */
    virtual BlockHashVecRes getChainByBlocks(
        const primitives::BlockHash &top_block,
        const primitives::BlockHash &bottom_block) = 0;

    /**
     * Get a longest path (chain of blocks) from the last finalized block down
     * to the deepest leaf
     * @return chain of blocks or error
     *
     * @note this function is equivalent to "getChainByBlock(deepestLeaf())"
     */
    virtual BlockHashVecRes longestPath() = 0;

    /**
     * Represents block information, including block number and hash
     */
    struct BlockInfo {
      primitives::BlockNumber block_number{};
      primitives::BlockHash block_hash{};
    };

    /**
     * Get a deepest leaf of the tree
     * @return deepest leaf
     *
     * @note deepest leaf is also a result of "SelectBestChain": if we are the
     * leader, we connect a block, which we constructed, to that deepest leaf
     */
    virtual BlockInfo deepestLeaf() const = 0;

    /**
     * Get all leaves of our tree
     * @return collection of the leaves
     */
    virtual std::vector<primitives::BlockHash> getLeaves() const = 0;

    /**
     * Get children of the block with specified hash
     * @param block to get children of
     * @return collection of children hashes or error
     */
    virtual BlockHashVecRes getChildren(const primitives::BlockHash &block) = 0;

    /**
     * Get the last finalized block
     * @return hash of the block
     */
    virtual primitives::BlockHash getLastFinalized() const = 0;

    /**
     * Prune the tree in both memory and storage, removing all blocks, which are
     * not descendants of the finalized blocks
     *
     * @note this function is called automatically when needed, but there's no
     * harm in calling it more often, than needed
     */
    virtual void prune() = 0;
  };
}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCK_TREE_HPP
