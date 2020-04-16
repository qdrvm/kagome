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

#ifndef KAGOME_BLOCKCHAIN_COMMON_HPP
#define KAGOME_BLOCKCHAIN_COMMON_HPP

#include <outcome/outcome.hpp>

#include "common/buffer.hpp"
#include "primitives/block_id.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::blockchain {
  using ReadableBufferMap =
      storage::face::Readable<common::Buffer, common::Buffer>;

  enum class Error {
    // it's important to convert storage errors of this type to this one to
    // enable a user to discern between cases when a header with provided id
    // is not found and when an internal storage error occurs
    BLOCK_NOT_FOUND = 1
  };

  /**
   * Convert a block ID into a key, which is a first part of a key, by which the
   * columns are stored in the database
   */
  outcome::result<common::Buffer> idToLookupKey(const ReadableBufferMap &map,
                                                const primitives::BlockId &id);

  /**
   * Instantiate empty merkle trie, insert \param key_vals pairs and \return
   * Buffer containing merkle root of resulting trie
   */
  common::Buffer trieRoot(
      const std::vector<std::pair<common::Buffer, common::Buffer>> &key_vals);
}  // namespace kagome::blockchain

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, Error)

#endif  // KAGOME_BLOCKCHAIN_COMMON_HPP
