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

#ifndef KAGOME_API_STATE_READONLY_TRIE_BUILDER_IMPL_HPP
#define KAGOME_API_STATE_READONLY_TRIE_BUILDER_IMPL_HPP

#include "api/state/readonly_trie_builder.hpp"
#include "storage/trie/trie_db_backend.hpp"

namespace kagome::api {

  class ReadonlyTrieBuilderImpl : public ReadonlyTrieBuilder {
   public:
    explicit ReadonlyTrieBuilderImpl(
        std::shared_ptr<storage::trie::TrieDbBackend> backend);
    ~ReadonlyTrieBuilderImpl() override = default;

    std::unique_ptr<storage::trie::TrieDbReader> buildAt(
        primitives::BlockHash state_root) const override;

   private:
    std::shared_ptr<storage::trie::TrieDbBackend> backend_;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_STATE_READONLY_TRIE_BUILDER_IMPL_HPP
