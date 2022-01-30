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

    /// Get hash of genesis block
    virtual outcome::result<primitives::BlockHash> getGenesisBlockHash()
        const = 0;

    virtual outcome::result<std::vector<primitives::BlockHash>>
    getBlockTreeLeaves() const = 0;
    virtual outcome::result<void> setBlockTreeLeaves(
        std::vector<primitives::BlockHash> leaves) = 0;

    // TODO(xDimon): After deploy of this change,
    //  getting of finalized block from storage should be removed
    [[deprecated]] virtual outcome::result<primitives::BlockHash>
    getLastFinalizedBlockHash() const = 0;
    [[deprecated]] virtual outcome::result<void> setLastFinalizedBlockHash(
        const primitives::BlockHash &) = 0;

    virtual outcome::result<bool> hasBlockHeader(
        const primitives::BlockId &id) const = 0;

    virtual outcome::result<primitives::BlockHeader> getBlockHeader(
        const primitives::BlockId &id) const = 0;
    virtual outcome::result<primitives::BlockBody> getBlockBody(
        const primitives::BlockId &id) const = 0;
    virtual outcome::result<primitives::BlockData> getBlockData(
        const primitives::BlockId &id) const = 0;
    virtual outcome::result<primitives::Justification> getJustification(
        const primitives::BlockId &block) const = 0;

    virtual outcome::result<primitives::BlockHash> putBlockHeader(
        const primitives::BlockHeader &header) = 0;

    virtual outcome::result<void> putBlockData(
        primitives::BlockNumber, const primitives::BlockData &block_data) = 0;

    virtual outcome::result<primitives::BlockHash> putBlock(
        const primitives::Block &block) = 0;

    virtual outcome::result<void> putJustification(
        const primitives::Justification &j,
        const primitives::BlockHash &hash,
        const primitives::BlockNumber &number) = 0;

    virtual outcome::result<void> removeBlock(
        const primitives::BlockHash &hash,
        const primitives::BlockNumber &number) = 0;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCK_STORAGE_HPP
