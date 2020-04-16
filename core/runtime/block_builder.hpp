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

#ifndef KAGOME_RUNTIME_BLOCK_BUILDER_HPP
#define KAGOME_RUNTIME_BLOCK_BUILDER_HPP

#include <list>

#include <outcome/outcome.hpp>
#include "primitives/apply_result.hpp"
#include "primitives/block.hpp"
#include "primitives/block_header.hpp"
#include "primitives/check_inherents_result.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/inherent_data.hpp"

namespace kagome::runtime {
  /**
   * Part of runtime API responsible for building a block for a runtime.
   */
  class BlockBuilder {
   public:
    virtual ~BlockBuilder() = default;

    /**
     * Apply the given extrinsics.
     */
    virtual outcome::result<primitives::ApplyResult> apply_extrinsic(
        const primitives::Extrinsic &extrinsic) = 0;

    /**
     * Finish the current block.
     */
    virtual outcome::result<primitives::BlockHeader> finalise_block() = 0;

    /**
     * Generate inherent extrinsics. The inherent data will vary from chain to
     * chain.
     */
    virtual outcome::result<std::vector<primitives::Extrinsic>>
    inherent_extrinsics(const primitives::InherentData &data) = 0;

    /**
     * Check that the inherents are valid. The inherent data will vary from
     * chain to chain.
     */
    virtual outcome::result<primitives::CheckInherentsResult> check_inherents(
        const primitives::Block &block,
        const primitives::InherentData &data) = 0;

    /**
     * Generate a random seed.
     */
    virtual outcome::result<common::Hash256> random_seed() = 0;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_BLOCK_BUILDER_HPP
