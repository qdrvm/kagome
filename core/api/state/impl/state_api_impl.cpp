/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/state/impl/state_api_impl.hpp"

#include <utility>

namespace kagome::api {

  StateApiImpl::StateApiImpl(
      std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
      std::shared_ptr<storage::trie::ReadonlyTrieFactory> trie_factory,
      std::shared_ptr<blockchain::BlockTree> block_tree)
      : block_repo_{std::move(block_repo)},
        trie_factory_{std::move(trie_factory)},
        block_tree_{std::move(block_tree)} {
    BOOST_ASSERT(block_repo_ != nullptr);
    BOOST_ASSERT(trie_factory_ != nullptr);
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
        trie_factory_->buildAt(header.state_root);
    return trie_reader->get(key);
  }
}  // namespace kagome::api
