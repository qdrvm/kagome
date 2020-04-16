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

#ifndef KAGOME_TRIE_DB_BACKEND_HPP
#define KAGOME_TRIE_DB_BACKEND_HPP

#include <outcome/outcome.hpp>

#include "common/buffer.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/trie/impl/polkadot_node.hpp"

namespace kagome::storage::trie {

  /**
   * Adapter for key-value storages that allows to hide keyspace separation
   * along with root hash storing logic from the trie db component
   */
  class TrieDbBackend : public BufferStorage {
   public:
    ~TrieDbBackend() override = default;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TRIE_DB_BACKEND_HPP
