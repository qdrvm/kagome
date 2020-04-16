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

#include "blockchain/block_tree_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain, BlockTreeError, e) {
  using E = kagome::blockchain::BlockTreeError;
  switch (e) {
    case E::INVALID_DB:
      return "genesis block is not provided, and the database is either empty "
             "or does not contain valid block tree";
    case E::NO_PARENT:
      return "block, which should have been added, has no known parent";
    case E::BLOCK_EXISTS:
      return "block, which should have been inserted, already exists in the "
             "tree";
    case E::HASH_FAILED:
      return "attempt to hash block part has failed";
    case E::NO_SUCH_BLOCK:
      return "block with such hash cannot be found in the local storage";
    case E::INCORRECT_ARGS:
      return "arguments, which were provided, are incorrect";
    case E::INTERNAL_ERROR:
      return "internal error happened";
  }
  return "unknown error";
}
