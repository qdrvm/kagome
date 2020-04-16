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

#ifndef KAGOME_BLOCK_VALIDATOR_HPP
#define KAGOME_BLOCK_VALIDATOR_HPP

#include <outcome/outcome.hpp>
#include "consensus/babe/types/epoch.hpp"
#include "primitives/block.hpp"

namespace kagome::consensus {
  /**
   * Validator of the blocks
   */
  class BlockValidator {
   public:
    virtual ~BlockValidator() = default;

    /**
     * Validate the block
     * @param block to be validated
     * @param authority_id authority that sent this block
     * @param threshold is vrf threshold for this epoch
     * @param randomness is randomness used in this epoch
     * @return nothing or validation error
     */
    virtual outcome::result<void> validateBlock(
        const primitives::Block &block,
        const primitives::AuthorityId &authority_id,
        const Threshold &threshold,
        const Randomness &randomness) const = 0;

    /**
     * Validate the block header
     * @param block to be validated
     * @param authority_id authority that sent this block
     * @param threshold is vrf threshold for this epoch
     * @param randomness is randomness used in this epoch
     * @return nothing or validation error
     */
    virtual outcome::result<void> validateHeader(
        const primitives::BlockHeader &block_header,
        const primitives::AuthorityId &authority_id,
        const Threshold &threshold,
        const Randomness &randomness) const = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BLOCK_VALIDATOR_HPP
