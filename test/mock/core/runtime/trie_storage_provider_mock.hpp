/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_TRIE_STORAGE_PROVIDER_MOCK
#define KAGOME_TEST_MOCK_CORE_RUNTIME_TRIE_STORAGE_PROVIDER_MOCK

#include "runtime/trie_storage_provider.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class TrieStorageProviderMock : public TrieStorageProvider {
   public:
    MOCK_METHOD(outcome::result<void>,
                setToEphemeralAt,
                (const storage::trie::RootHash &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                setToPersistentAt,
                (const storage::trie::RootHash &),
                (override));

    MOCK_METHOD(std::shared_ptr<Batch>, getCurrentBatch, (), (const, override));

    MOCK_METHOD(outcome::result<std::shared_ptr<PersistentBatch>>,
                getChildBatchAt,
                (const common::Buffer &),
                (override));

    MOCK_METHOD(outcome::result<storage::trie::RootHash>,
                forceCommit,
                (StateVersion),
                (override));

    MOCK_METHOD(outcome::result<void>, startTransaction, (), (override));

    MOCK_METHOD(outcome::result<void>, rollbackTransaction, (), (override));

    MOCK_METHOD(outcome::result<void>, commitTransaction, (), (override));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_TRIE_STORAGE_PROVIDER_MOCK
