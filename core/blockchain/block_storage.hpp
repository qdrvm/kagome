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

    /// Check if header existing by provided block {@param id}
    virtual outcome::result<bool> hasBlockHeader(
        const primitives::BlockId &id) const = 0;

    /**
     * Tries to get block header by {@param id}
     * @returns block header or error
     */
    virtual outcome::result<std::optional<primitives::BlockHeader>> getBlockHeader(
        const primitives::BlockId &id) const = 0;

    /**
     * Tries to get block body by {@param id}
     * @returns block body or error
     */
    virtual outcome::result<std::optional<primitives::BlockBody>> getBlockBody(
        const primitives::BlockId &id) const = 0;

    /**
     * Tries to get block data by {@param id}
     * @returns block data or error
     */
    virtual outcome::result<std::optional<primitives::BlockData>> getBlockData(
        const primitives::BlockId &id) const = 0;

    /**
     * Tries to get justification of block finality by {@param id}
     * @returns justification or error
     */
    virtual outcome::result<std::optional<primitives::Justification>> getJustification(
        const primitives::BlockId &block) const = 0;

    /**
     * Saves number-to-block_lookup_key for {@param block} to block storage
     * @returns hash of saved header or error
     */
    virtual outcome::result<void> putNumberToIndexKey(
        const primitives::BlockInfo &block) = 0;

    /**
     * Saves block header {@param header} to block storage
     * @returns hash of saved header or error
     */
    virtual outcome::result<primitives::BlockHash> putBlockHeader(
        const primitives::BlockHeader &header) = 0;

    /**
     * Saves {@param data} of block with {@param number} to block storage
     * @returns result of saving
     */
    virtual outcome::result<void> putBlockData(
        primitives::BlockNumber, const primitives::BlockData &block_data) = 0;

    /**
     * Saves {@param block} to block storage
     * @returns hash of saved header or error
     */
    virtual outcome::result<primitives::BlockHash> putBlock(
        const primitives::Block &block) = 0;

    /**
     * Saves {@param justification} of block with {@param number} and {@param
     * hash} to block storage
     * @returns result of saving
     */
    virtual outcome::result<void> putJustification(
        const primitives::Justification &justification,
        const primitives::BlockHash &hash,
        const primitives::BlockNumber &number) = 0;

    /**
     * Removes all data of block {@param block} from block storage
     * @returns result of removing
     */
    virtual outcome::result<void> removeBlock(
        const primitives::BlockInfo &block) = 0;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCK_STORAGE_HPP
