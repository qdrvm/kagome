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
    MOCK_METHOD0(setToEphemeral, outcome::result<void>());
    MOCK_METHOD1(setToEphemeralAt,
                 outcome::result<void>(const storage::trie::RootHash &));
    MOCK_METHOD0(setToPersistent, outcome::result<void>());
    MOCK_METHOD1(setToPersistentAt,
                 outcome::result<void>(const storage::trie::RootHash &));
    MOCK_CONST_METHOD0(getCurrentBatch, std::shared_ptr<Batch>());
    MOCK_CONST_METHOD0(tryGetPersistentBatch,
                       std::optional<std::shared_ptr<PersistentBatch>>());
    MOCK_CONST_METHOD0(isCurrentlyPersistent, bool());
    MOCK_METHOD0(forceCommit, outcome::result<storage::trie::RootHash>());
    MOCK_METHOD0(startTransaction, outcome::result<void>());
    MOCK_METHOD0(rollbackTransaction, outcome::result<void>());
    MOCK_METHOD0(commitTransaction, outcome::result<void>());
    MOCK_CONST_METHOD0(getLatestRootMock, storage::trie::RootHash());
    storage::trie::RootHash getLatestRoot() const noexcept override {
      return getLatestRootMock();
    }
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_TRIE_STORAGE_PROVIDER_MOCK
