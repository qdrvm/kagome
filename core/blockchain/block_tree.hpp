/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "consensus/timeline/types.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block.hpp"
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"
#include "primitives/justification.hpp"
#include "primitives/version.hpp"

namespace kagome::blockchain {
  /**
   * Storage for blocks, which has a form of tree; it serves two functions:
   *    - keep tracking of all finalized blocks (they are kept in the
   *      non-volatile storage)
   *    - work with blocks, which participate in the current round of BABE block
   *      production (handling forks, pruning the blocks, resolving child-parent
   *      relations, etc)
   */
  class BlockTree {
   public:
    using BlockHashVecRes = outcome::result<std::vector<primitives::BlockHash>>;

    virtual ~BlockTree() = default;

    /**
     * @returns hash of genesis block
     */
    virtual const primitives::BlockHash &getGenesisBlockHash() const = 0;

    /**
     * Get block hash by provided block number
     * @param block_number of the block header we are looking for
     * @return result containing block hash if it exists, error otherwise
     */
    virtual outcome::result<std::optional<primitives::BlockHash>> getBlockHash(
        primitives::BlockNumber block_number) const = 0;

    /**
     * Checks containing of block header by provided block id
     * @param block_hash hash of the block header we are checking
     * @return containing block header or does not, or error
     */
    virtual outcome::result<bool> hasBlockHeader(
        const primitives::BlockHash &block_hash) const = 0;

    /**
     * Get block header by provided block id
     * @param block_hash hash of the block header we are looking for
     * @return result containing block header if it exists, error otherwise
     */
    virtual outcome::result<primitives::BlockHeader> getBlockHeader(
        const primitives::BlockHash &block_hash) const = 0;

    /**
     * Get a body (extrinsics) of the block (if present)
     * @param block_hash hash of the block to get body for
     * @return body, if the block exists in our storage, error in case it does
     * not exist in our storage, or actual error happens
     */
    virtual outcome::result<primitives::BlockBody> getBlockBody(
        const primitives::BlockHash &block_hash) const = 0;

    /**
     * Get a justification of the block (if present)
     * @param block_hash hash of the block to get justification for
     * @return body, if the block exists in our storage, error in case it does
     * not exist in our storage, or actual error happens
     */
    virtual outcome::result<primitives::Justification> getBlockJustification(
        const primitives::BlockHash &block_hash) const = 0;

    /**
     * Adds header to the storage
     * @param header that we are adding
     * @return result with success if header's parent exists on storage and new
     * header was added. Error otherwise
     */
    virtual outcome::result<void> addBlockHeader(
        const primitives::BlockHeader &header) = 0;

    /**
     * Adds block body to the storage
     * @param block_number that corresponds to the block which body we are
     * adding
     * @param block_hash that corresponds to the block which body we are adding
     * @param block_body that we are adding
     * @return result with success if block body was inserted. Error otherwise
     */
    virtual outcome::result<void> addBlockBody(
        const primitives::BlockHash &block_hash,
        const primitives::BlockBody &block_body) = 0;

    /**
     * Add an existent block to the tree
     * @param block_hash is hash of the added block in the tree
     * @param block_header is header of that block
     * @return nothing or error; if error happens, no changes in the tree are
     * made
     */
    virtual outcome::result<void> addExistingBlock(
        const primitives::BlockHash &block_hash,
        const primitives::BlockHeader &block_header) = 0;

    /**
     * Adjusts weight for the block as contained parachain data.
     * @param block_hash is hash of the weighted block
     */
    virtual outcome::result<void> markAsParachainDataBlock(
        const primitives::BlockHash &block_hash) = 0;

    /**
     * The passed blocks will be marked as reverted, and their descendants will
     * be marked as non-viable
     * @param block_hashes is vector of reverted block hashes
     */
    virtual outcome::result<void> markAsRevertedBlocks(
        const std::vector<primitives::BlockHash> &block_hashes) = 0;

    /**
     * Add a new block to the tree
     * @param block to be stored and added to tree
     * @return nothing or error; if error happens, no changes in the tree are
     * made
     *
     * @note if block, which is specified in PARENT_HASH field of (\param block)
     * is not in our local storage, corresponding error is returned. It is
     * suggested that after getting that error, the caller would ask another
     * peer for the parent block and try to insert it; this operation is to be
     * repeated until a successful insertion happens
     */
    virtual outcome::result<void> addBlock(const primitives::Block &block) = 0;

    /**
     * Remove leaf
     * @param block_hash - hash of block to be deleted. The block must be leaf.
     * @return nothing or error
     */
    virtual outcome::result<void> removeLeaf(
        const primitives::BlockHash &block_hash) = 0;

    /**
     * Mark the block as finalized and store a finalization justification
     * @param block to be finalized
     * @param justification of the finalization
     * @return nothing or error
     */
    virtual outcome::result<void> finalize(
        const primitives::BlockHash &block,
        const primitives::Justification &justification) = 0;

    enum class GetChainDirection { ASCEND, DESCEND };

    /**
     * Get a chain of blocks from provided block to best block direction
     * @param block, from which the chain is started
     * @param maximum number of blocks to be retrieved
     * @return chain or blocks or error
     */
    virtual BlockHashVecRes getBestChainFromBlock(
        const primitives::BlockHash &block, uint64_t maximum) const = 0;

    /**
     * Get a chain of blocks before provided block including its
     * @param block, to which the chain is ended
     * @param maximum number of blocks to be retrieved
     * @return chain or blocks or error
     */
    virtual BlockHashVecRes getDescendingChainToBlock(
        const primitives::BlockHash &block, uint64_t maximum) const = 0;

    /**
     * Get a chain of blocks.
     * Implies `hasDirectChain(ancestor, descendant)`.
     * @param ancestor - block, which is closest to the genesis
     * @param descendant - block, which is farthest from the genesis
     * @return chain of blocks in ascending order or error
     */
    virtual BlockHashVecRes getChainByBlocks(
        const primitives::BlockHash &ancestor,
        const primitives::BlockHash &descendant) const = 0;

    /**
     * Check if one block is ancestor of second one (direct chain exists)
     * @param ancestor - block, which is closest to the genesis
     * @param descendant - block, which is farthest from the genesis
     * @return true if \param ancestor is ancestor of \param descendant
     */
    virtual bool hasDirectChain(
        const primitives::BlockHash &ancestor,
        const primitives::BlockHash &descendant) const = 0;

    bool hasDirectChain(const primitives::BlockInfo &ancestor,
                        const primitives::BlockInfo &descendant) const {
      return hasDirectChain(ancestor.hash, descendant.hash);
    }

    virtual bool isFinalized(const primitives::BlockInfo &block) const = 0;

    /**
     * Get a best leaf of the tree
     * @return best leaf
     *
     * @note best block is also a result of "SelectBestChain": if we are the
     * leader, we connect a block, which we constructed, to that best block
     */
    virtual primitives::BlockInfo bestBlock() const = 0;

    /**
     * @brief Get the most recent block of the best (longest) chain among
     * those that contain a block with \param target_hash
     * @param target_hash is a hash of a block that the chosen chain must
     * contain
     * @param max_number is the max block number that the resulting block (and
     * the target one) may possess
     */
    virtual outcome::result<primitives::BlockInfo> getBestContaining(
        const primitives::BlockHash &target_hash) const = 0;

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
    virtual BlockHashVecRes getChildren(
        const primitives::BlockHash &block) const = 0;

    /**
     * Get the last finalized block
     * @return hash of the block
     */
    virtual primitives::BlockInfo getLastFinalized() const = 0;

    /**
     * Warp synced to block.
     */
    virtual void warp(const primitives::BlockInfo &block) = 0;

    /**
     * Notify best and finalized block to subscriptions.
     */
    virtual void notifyBestAndFinalized() = 0;
  };

}  // namespace kagome::blockchain
