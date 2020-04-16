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

#ifndef KAGOME_TRIE_DB_READER_HPP
#define KAGOME_TRIE_DB_READER_HPP

#include "common/buffer.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::storage::trie {

  /**
   * Provides read access for Trie DB - cryptographically authenticated
   * key-value storage
   */
  class TrieDbReader : public virtual ReadOnlyBufferMap {
   public:
    /**
     * @brief Calculate and return trie root.
     * @return byte buffer of any size (different hashing algorithms may be
     * used)
     */
    virtual Buffer getRootHash() const = 0;

    /**
     * @returns true if the trie is empty, false otherwise
     */
    virtual bool empty() const = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TRIE_DB_READER_HPP
