/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/state/impl/state_api_impl.hpp"

#include <utility>

namespace kagome::api {

  StateApiImpl::StateApiImpl(
      std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
      std::shared_ptr<const storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::Core> r_core)
      : block_repo_{std::move(block_repo)},
        storage_{std::move(trie_storage)},
        block_tree_{std::move(block_tree)},
        r_core_{std::move(r_core)} {
    BOOST_ASSERT(nullptr != block_repo_);
    BOOST_ASSERT(nullptr != storage_);
    BOOST_ASSERT(nullptr != block_tree_);
    BOOST_ASSERT(nullptr != r_core_);
  }

  outcome::result<common::Buffer> StateApiImpl::getStorage(
      const common::Buffer &key) const {
    auto last_finalized = block_tree_->getLastFinalized();
    return getStorage(key, last_finalized.block_hash);
  }

  outcome::result<common::Buffer> StateApiImpl::getStorage(
      const common::Buffer &key, const primitives::BlockHash &at) const {
    OUTCOME_TRY(header, block_repo_->getBlockHeader(at));
    OUTCOME_TRY(trie_reader, storage_->getEphemeralBatchAt(header.state_root));
    return trie_reader->get(key);
  }

  outcome::result<primitives::Version> StateApiImpl::getRuntimeVersion(
      const boost::optional<primitives::BlockHash> &at) const {
    return r_core_->version(at);
  }
}  // namespace kagome::api
