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

#ifndef KAGOME_TRIE_TRIE_DB_HPP
#define KAGOME_TRIE_TRIE_DB_HPP

#include "common/buffer.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/trie/trie_db_reader.hpp"

namespace kagome::storage::trie {

  /**
   * @brief Represents a cryptographically authenticated key-value
   * storage - Trie DB, backed by a key-value database.
   */
  class TrieDb : public TrieDbReader, public BatchWriteBufferMap {
   public:
    /**
     * remove storage entries which keys start with given prefix
     */
    virtual outcome::result<void> clearPrefix(const common::Buffer &buf) = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TRIE_TRIE_DB_HPP
