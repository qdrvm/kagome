/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_STORAGE_HPP
#define KAGOME_BLOCK_STORAGE_HPP

#include "primitives/block.hpp"
#include "primitives/block_data.hpp"
#include "primitives/block_id.hpp"
#include "primitives/justification.hpp"

namespace kagome::blockchain {

  /**
   * A wrapper for a storage of blocks
   * Provides a convenient interface to work with it
   */
  class BlockStorage {
   public:
    virtual ~BlockStorage() = default;

    /**
     * Obtains leaves of block tree
     * @returns hashes of block tree leaves
     */
    virtual outcome::result<std::vector<primitives::BlockHash>>
    getBlockTreeLeaves() const = 0;

    /**
     * Saves provided block tree {@param leaves}
     * @returns result of saving
     */
    virtual outcome::result<void> setBlockTreeLeaves(
        std::vector<primitives::BlockHash> leaves) = 0;

    /**
     * Get the last finalized block
     * @return BlockInfo of the block
     */
    virtual outcome::result<primitives::BlockInfo> getLastFinalized() const = 0;

    // -- hash --

    /**
     * Saves number-to-hash record for provided {@param block_info} to block
     * storage
     * @returns success or failure
     */
    virtual outcome::result<void> assignNumberToHash(
        const primitives::BlockInfo &block_info) = 0;

    /**
     * Removes number-to-hash record for provided {@param block_number} from
     * block storage
     * @returns success or failure
     */
    virtual outcome::result<void> deassignNumberToHash(
        primitives::BlockNumber block_number) = 0;

    /**
     * Tries to get block hash by number {@param block_number}
     * @returns hash or error
     */
    virtual outcome::result<std::optional<primitives::BlockHash>> getBlockHash(
        primitives::BlockNumber block_number) const = 0;

    /**
     * Tries to get block hash by id {@param block_id}
     * @returns hash or error
     */
    virtual outcome::result<std::optional<primitives::BlockHash>> getBlockHash(
        const primitives::BlockId &block_id) const = 0;

    // -- headers --

    /**
     * Check if header existing by provided block {@param block_hash}
     * @returns result or error
     */
    virtual outcome::result<bool> hasBlockHeader(
        const primitives::BlockHash &block_hash) const = 0;

    /**
     * Saves block header {@param header} to block storage
     * @returns hash of saved header or error
     */
    virtual outcome::result<primitives::BlockHash> putBlockHeader(
        const primitives::BlockHeader &header) = 0;

    /**
     * Tries to get block header by {@param block_hash}
     * @returns block header or error
     */
    virtual outcome::result<std::optional<primitives::BlockHeader>>
    getBlockHeader(const primitives::BlockHash &block_hash) const = 0;

    // -- body --

    /**
     * Saves provided body {@param block_body} of block {@param block_hash} to
     * block storage
     * @returns result of saving
     */
    virtual outcome::result<void> putBlockBody(
        const primitives::BlockHash &block_hash,
        const primitives::BlockBody &block_body) = 0;

    /**
     * Tries to get block body by {@param block_hash}
     * @returns block body or error
     */
    virtual outcome::result<std::optional<primitives::BlockBody>> getBlockBody(
        const primitives::BlockHash &block_hash) const = 0;

    /**
     * Removes body of block with hash {@param block_hash} from block storage
     * @returns result of saving
     */
    virtual outcome::result<void> removeBlockBody(
        const primitives::BlockHash &block_hash) = 0;

    // -- justification --

    /**
     * Saves {@param justification} of block with hash {@param block_hash} to
     * block storage
     * @returns result of saving
     */
    virtual outcome::result<void> putJustification(
        const primitives::Justification &justification,
        const primitives::BlockHash &block_hash) = 0;

    /**
     * Tries to get justification of block finality by {@param block_hash}
     * @returns justification or error
     */
    virtual outcome::result<std::optional<primitives::Justification>>
    getJustification(const primitives::BlockHash &block_hash) const = 0;

    /**
     * Removes justification of block with hash {@param block_hash} from block
     * storage
     * @returns result of saving
     */
    virtual outcome::result<void> removeJustification(
        const primitives::BlockHash &block_hash) = 0;

    // -- combined

    /**
     * Saves block {@param block} to block storage
     * @returns hash of saved header or error
     */
    virtual outcome::result<primitives::BlockHash> putBlock(
        const primitives::Block &block) = 0;

    /**
     * Tries to get block data by {@param block_hash}
     * @returns block data or error
     */
    virtual outcome::result<std::optional<primitives::BlockData>> getBlockData(
        const primitives::BlockHash &block_hash) const = 0;

    /**
     * Removes all data of block with hash {@param block_hash} from block
     * storage
     * @returns result of removing
     */
    virtual outcome::result<void> removeBlock(
        const primitives::BlockHash &block_hash) = 0;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCK_STORAGE_HPP
