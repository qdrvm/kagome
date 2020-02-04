/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_STORAGE_HPP
#define KAGOME_BLOCK_STORAGE_HPP

#include "primitives/block.hpp"
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

    virtual outcome::result<primitives::BlockHeader> getBlockHeader(
        const primitives::BlockId &id) const = 0;
    virtual outcome::result<primitives::BlockBody> getBlockBody(
        const primitives::BlockId &id) const = 0;
    virtual outcome::result<primitives::Justification> getJustification(
        const primitives::BlockId &block) const = 0;

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
