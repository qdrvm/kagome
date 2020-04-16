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

#ifndef KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_FACTORY_HPP
#define KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_FACTORY_HPP

#include <vector>

#include "authorship/block_builder.hpp"
#include "primitives/block_id.hpp"
#include "runtime/block_builder.hpp"
#include "runtime/core.hpp"

namespace kagome::authorship {

  /**
   * Creates new block builders. Each of them encapsulates the logic for
   * creating a single block from provided block information
   */
  class BlockBuilderFactory {
   public:
    virtual ~BlockBuilderFactory() = default;

    /**
     * Prepares BlockBuilder for creating block on top of parent block and using
     * provided digests. Also initialises the block created in BlockBuilder
     */
    virtual outcome::result<std::unique_ptr<BlockBuilder>> create(
        const primitives::BlockId &parent_id,
        primitives::Digest inherent_digest) const = 0;
  };

}  // namespace kagome::authorship

#endif  // KAGOME_CORE_AUTHORSHIP_BLOCK_BUILDER_FACTORY_HPP
