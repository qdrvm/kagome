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

#ifndef KAGOME_BLOCK_TREE_ERROR_HPP
#define KAGOME_BLOCK_TREE_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::blockchain {
  /**
   * Errors of the block tree are here, so that other modules can use them, for
   * example, to compare a received error with those
   */
  enum class BlockTreeError {
    INVALID_DB = 1,
    NO_PARENT,
    BLOCK_EXISTS,
    HASH_FAILED,
    NO_SUCH_BLOCK,
    INCORRECT_ARGS,
    INTERNAL_ERROR
  };
}  // namespace kagome::blockchain

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, BlockTreeError)

#endif  // KAGOME_BLOCK_TREE_ERROR_HPP
