/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/state/impl/readonly_trie_builder_impl.hpp"

#include "storage/trie/impl/polkadot_trie_db.hpp"

namespace kagome::api {

  ReadonlyTrieBuilderImpl::ReadonlyTrieBuilderImpl(
      std::shared_ptr<storage::trie::TrieDbBackend> backend):
      backend_ {std::move(backend)} {
    BOOST_ASSERT(backend_ != nullptr);
  }


  std::unique_ptr<storage::trie::TrieDbReader> ReadonlyTrieBuilderImpl::buildAt(
      primitives::BlockHash state_root) const {
    return storage::trie::PolkadotTrieDb::initReadOnlyFromStorage(
        common::Buffer{state_root}, backend_);
  }

}
