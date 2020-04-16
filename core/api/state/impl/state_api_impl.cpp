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

#include "api/state/impl/state_api_impl.hpp"

#include <utility>

namespace kagome::api {

  StateApiImpl::StateApiImpl(
      std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
      std::shared_ptr<ReadonlyTrieBuilder> trie_builder,
      std::shared_ptr<blockchain::BlockTree> block_tree)
      : block_repo_{std::move(block_repo)},
        trie_builder_{std::move(trie_builder)},
        block_tree_{std::move(block_tree)} {
    BOOST_ASSERT(block_repo_ != nullptr);
    BOOST_ASSERT(trie_builder_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
  }

  outcome::result<common::Buffer> StateApiImpl::getStorage(
      const common::Buffer &key) const {
    auto last_finalized = block_tree_->getLastFinalized();
    return getStorage(key, last_finalized.block_hash);
  }

  outcome::result<common::Buffer> StateApiImpl::getStorage(
      const common::Buffer &key, const primitives::BlockHash &at) const {
    OUTCOME_TRY(header, block_repo_->getBlockHeader(at));
    auto trie_reader =
        trie_builder_->buildAt(header.state_root);
    return trie_reader->get(key);
  }
}  // namespace kagome::api
