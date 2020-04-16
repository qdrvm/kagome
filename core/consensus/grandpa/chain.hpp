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

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_CHAIN_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_CHAIN_HPP

#include <vector>

#include <boost/optional.hpp>
#include <outcome/outcome.hpp>
#include "consensus/grandpa/common.hpp"
#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Chain context necessary for implementation of the finality gadget.
   */
  struct Chain {
    virtual ~Chain() = default;

    /**
     * @brief Get the ancestry of a {@param block} up to but not including the
     * {@param base} hash. Should be in reverse order from block's parent.
     * @return If the block is not a descendent of base, returns an error.
     */
    virtual outcome::result<std::vector<primitives::BlockHash>> getAncestry(
        const primitives::BlockHash &base,
        const primitives::BlockHash &block) const = 0;

    /**
     * @returns the hash of the best block whose chain contains the given
     * block hash, even if that block is {@param base} itself. If base is
     * unknown, return None.
     */
    virtual outcome::result<BlockInfo> bestChainContaining(
        const primitives::BlockHash &base) const = 0;

    /**
     * @returns true if {@param block} is a descendent of or equal to the
     * given {@param base}.
     */
    inline bool isEqualOrDescendOf(const primitives::BlockHash &base,
                            const primitives::BlockHash &block) const {
      return base == block ? true : getAncestry(base, block).has_value();
    }
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_CHAIN_HPP
