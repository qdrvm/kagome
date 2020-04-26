/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/in_memory_trie_db_factory.hpp"

#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/impl/polkadot_trie_db.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"

namespace kagome::storage::trie {

  std::unique_ptr<TrieDb> InMemoryTrieDbFactory::makeTrieDb() const {
    return PolkadotTrieDb::createEmpty(std::make_shared<TrieDbBackendImpl>(
        std::make_shared<InMemoryStorage>(), common::Buffer{}));
  }

}  // namespace kagome::storage::trie
