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
    MOCK_METHOD0(getPersistentBatch,
                 outcome::result<std::unique_ptr<PersistentTrieBatch>>());
    MOCK_CONST_METHOD0(getEphemeralBatch,
                       outcome::result<std::unique_ptr<EphemeralTrieBatch>>());

    MOCK_METHOD1(getPersistentBatchAt,
                 outcome::result<std::unique_ptr<PersistentTrieBatch>>(
                     const common::Hash256 &root));
    MOCK_CONST_METHOD1(getEphemeralBatchAt,
                       outcome::result<std::unique_ptr<EphemeralTrieBatch>>(
                           const common::Hash256 &root));

    MOCK_CONST_METHOD0(getRootHash, common::Buffer());
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_DB_MOCK_HPP
