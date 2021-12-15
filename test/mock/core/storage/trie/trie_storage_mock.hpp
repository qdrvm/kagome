/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_STORAGE_MOCK
#define KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_STORAGE_MOCK

#include <gmock/gmock.h>

#include "storage/trie/trie_storage.hpp"

namespace kagome::storage::trie {

  class TrieStorageMock : public TrieStorage {
   public:
    MOCK_METHOD(outcome::result<std::unique_ptr<PersistentTrieBatch>>,
                getPersistentBatchAt,
                (const storage::trie::RootHash &root),
                (override));

    MOCK_METHOD(outcome::result<std::unique_ptr<EphemeralTrieBatch>>,
                getEphemeralBatchAt,
                (const storage::trie::RootHash &root),
                (const, override));
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_DB_MOCK_HPP
