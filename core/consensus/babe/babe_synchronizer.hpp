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

#ifndef KAGOME_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_HPP
#define KAGOME_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_HPP

#include "primitives/block.hpp"
#include "primitives/block_id.hpp"

namespace kagome::consensus {

  /**
   * @brief Iterates over the list of accessible peers and tries to fetch
   * missing blocks from them
   */
  class BabeSynchronizer {
   public:
    using BlocksHandler =
        std::function<void(const std::vector<primitives::Block> &)>;

    virtual ~BabeSynchronizer() = default;

    /**
     * Request blocks between provided ones
     * @param from block id of the first requested block
     * @param to block hash of the last requested block
     * @param block_list_handler handles received blocks
     */
    virtual void request(const primitives::BlockId &from,
                         const primitives::BlockHash &to,
                         const BlocksHandler &block_list_handler) = 0;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_HPP
