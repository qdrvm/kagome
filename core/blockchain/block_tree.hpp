/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_TREE_HPP
#define KAGOME_BLOCK_TREE_HPP

#include <boost/optional.hpp>
#include <cstdint>
#include <vector>

#include "consensus/babe/types/epoch_digest.hpp"
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
     * Get block header by provided block id
     * @param block id of the block header we are looking for
     * @return result containing block header if it exists, error otherwise
     */
    virtual outcome::result<primitives::BlockHeader> getBlockHeader(
        const primitives::BlockId &block) const = 0;

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
     * Method to get actual runtime version.
     * @return runtime version.
     */
    virtual boost::optional<primitives::Version> runtimeVersion() const = 0;

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
        primitives::BlockNumber block_number,
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
        const primitives::BlockHash &block) const = 0;

    enum class GetChainDirection {
      ASCEND,
      DESCEND
    };

    /**
     * Get a chain of blocks from the (\param block)
     * @param block, from which the chain is started
     * @param ascending - if true, the chain will grow up from the provided
     * block (it is the lowest one); if false, down
     * @param maximum number of blocks to be retrieved
     * @return chain or blocks or error
     */
    virtual BlockHashVecRes getChainByBlock(const primitives::BlockHash &block,
                                            GetChainDirection ascending,
                                            uint64_t maximum) const = 0;

    /**
     * Get a chain of blocks
     * @param top_block - block, which is at the top of the chain
     * @param bottom_block - block, which is the bottom of the chain
     * @return chain of blocks in top-to-bottom order or error
     */
    virtual BlockHashVecRes getChainByBlocks(
        const primitives::BlockHash &top_block,
        const primitives::BlockHash &bottom_block) const = 0;

    /**
     * Get a chain of blocks
     * @param top_block - block, which is at the top of the chain
     * @param bottom_block - block, which is the bottom of the chain
     * @param max_count - maximum blocks in the chain
     * @return chain of blocks in top-to-bottom order or error
     */
    virtual BlockHashVecRes getChainByBlocks(
        const primitives::BlockHash &top_block,
        const primitives::BlockHash &bottom_block,
        const uint32_t max_count) const = 0;

    /**
     * Check if one block is ancestor of second one (direct chain exists)
     * @param ancestor - block, which is at the top of the chain
     * @param descendant - block, which is the bottom of the chain
     * @return true if \param ancestor is ancestor of \param descendant
     */
    virtual bool hasDirectChain(const primitives::BlockHash &ancestor,
                                const primitives::BlockHash &descendant) const = 0;

    /**
     * Get a longest path (chain of blocks) from the last finalized block down
     * to the deepest leaf
     * @return chain of blocks or error
     *
     * @note this function is equivalent to "getChainByBlock(deepestLeaf())"
     */
    virtual BlockHashVecRes longestPath() const = 0;

    /**
     * Get a deepest leaf of the tree
     * @return deepest leaf
     *
     * @note deepest leaf is also a result of "SelectBestChain": if we are the
     * leader, we connect a block, which we constructed, to that deepest leaf
     */
    virtual primitives::BlockInfo deepestLeaf() const = 0;

    /**
     * @brief Get the most recent block of the best (longest) chain among those
     * that contain a block with \param target_hash
     * @param target_hash is a hash of a block that the chosen chain must
     * contain
     * @param max_number is the max block number that the resulting block (and
     * the target one) may possess
     */
    virtual outcome::result<primitives::BlockInfo> getBestContaining(
        const primitives::BlockHash &target_hash,
        const boost::optional<primitives::BlockNumber> &max_number) const = 0;

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
    virtual primitives::BlockInfo getLastFinalized() const = 0;

    /**
     * Finds epoch descriptor for epoch with index {@param epoch_number}.
     * Search starts of block with hash {@param block_hash}.
     * @returns epoch descriptor, or error if it impossible.
     */
    virtual outcome::result<consensus::EpochDigest> getEpochDescriptor(
        consensus::EpochNumber epoch_number,
        primitives::BlockHash block_hash) const = 0;
  };
}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCK_TREE_HPP
