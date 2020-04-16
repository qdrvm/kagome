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

#ifndef KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_HPP
#define KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_HPP

#include "primitives/block.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::authorship {

  /**
   * BlockBuilder collects extrinsics and creates new block and then should be
   * destroyed
   */
  class BlockBuilder {
   public:
    virtual ~BlockBuilder() = default;

    /**
     * Push extrinsic to wait its inclusion to the block
     * Returns result containing success if xt was pushed, error otherwise
     */
    virtual outcome::result<void> pushExtrinsic(
        const primitives::Extrinsic &extrinsic) = 0;

    /**
     * Create a block from extrinsics and header
     */
    virtual outcome::result<primitives::Block> bake() const = 0;
  };

}  // namespace kagome::authorship

#endif  // KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_HPP
