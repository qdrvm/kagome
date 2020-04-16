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

#ifndef KAGOME_CORE_AUTHORSHIP_PROPOSER_TEST_HPP
#define KAGOME_CORE_AUTHORSHIP_PROPOSER_TEST_HPP

#include "clock/clock.hpp"
#include "primitives/block.hpp"
#include "primitives/block_id.hpp"
#include "primitives/digest.hpp"
#include "primitives/inherent_data.hpp"

namespace kagome::authorship {

  /**
   * Create block to further proposal for consensus
   */
  class Proposer {
   public:
    virtual ~Proposer() = default;

    /**
     * Creates block from provided parameters
     * @param parent_block_id hash or number of parent
     * @param inherent_data additional data on block from unsigned extrinsics
     * @param inherent_digests - chain-specific block auxilary data
     * @return proposed block or error
     */
    virtual outcome::result<primitives::Block> propose(
        const primitives::BlockId &parent_block_id,
        const primitives::InherentData &inherent_data,
        const primitives::Digest &inherent_digest) = 0;
  };

}  // namespace kagome::authorship

#endif  // KAGOME_CORE_AUTHORSHIP_PROPOSER_TEST_HPP
