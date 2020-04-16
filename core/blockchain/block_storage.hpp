/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
