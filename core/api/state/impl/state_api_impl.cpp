/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/state/impl/state_api_impl.hpp"

namespace kagome::api {

  outcome::result<common::Buffer> StateApiImpl::getStorage(
      const common::Buffer &key) {
    auto last_finalized = block_tree_->getLastFinalized();
    return getStorage(key, last_finalized);
  }

  outcome::result<common::Buffer> StateApiImpl::getStorage(
      const common::Buffer &key, const primitives::BlockHash &at) {
    OUTCOME_TRY(header, block_repo_->getBlockHeader(at));
    auto trie_reader = storage::trie::PolkadotTrieDb::initReadOnlyFromStorage(
        common::Buffer{header.state_root}, trie_backend_);
    return trie_reader->get(key);
  }
}  // namespace kagome::api
