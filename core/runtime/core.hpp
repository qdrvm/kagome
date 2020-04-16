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

#ifndef KAGOME_RUNTIME_CORE_HPP
#define KAGOME_RUNTIME_CORE_HPP

#include <outcome/outcome.hpp>
#include <vector>

#include "primitives/authority.hpp"
#include "primitives/block.hpp"
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"
#include "primitives/transaction_validity.hpp"
#include "primitives/version.hpp"

namespace kagome::runtime {
  /**
   * Core represents mandatory part of runtime api
   */
  class Core {
   public:
    virtual ~Core() = default;

    /**
     * @brief Returns the version of the runtime
     * @return runtime version
     */
    virtual outcome::result<primitives::Version> version() = 0;

    /**
     * @brief Executes the given block
     * @param block block to execute
     */
    virtual outcome::result<void> execute_block(
        const primitives::Block &block) = 0;

    /**
     * @brief Initialize a block with the given header.
     * @param header header used for block initialization
     */
    virtual outcome::result<void> initialise_block(
        const primitives::BlockHeader &header) = 0;

    /**
     * Get current authorities
     * @return collection of authorities
     */
    virtual outcome::result<std::vector<primitives::AuthorityId>> authorities(
        const primitives::BlockId &block_id) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_CORE_HPP
