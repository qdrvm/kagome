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

#ifndef KAGOME_STATE_API_IMPL_HPP
#define KAGOME_STATE_API_IMPL_HPP

#include "api/state/state_api.hpp"
#include "api/state/readonly_trie_builder.hpp"
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"

namespace kagome::api {

  class StateApiImpl : public StateApi {
   public:
    StateApiImpl(std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
                 std::shared_ptr<ReadonlyTrieBuilder> trie_builder,
                 std::shared_ptr<blockchain::BlockTree> block_tree);

    outcome::result<common::Buffer> getStorage(
        const common::Buffer &key) const override;
    outcome::result<common::Buffer> getStorage(
        const common::Buffer &key, const primitives::BlockHash &at) const override;

   private:
    std::shared_ptr<blockchain::BlockHeaderRepository> block_repo_;
    std::shared_ptr<ReadonlyTrieBuilder> trie_builder_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
  };

}  // namespace kagome::api

#endif  // KAGOME_STATE_API_IMPL_HPP
