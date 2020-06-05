/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_TRIE_STORAGE_PROVIDER_MOCK
#define KAGOME_TEST_MOCK_CORE_RUNTIME_TRIE_STORAGE_PROVIDER_MOCK

#include "runtime/trie_storage_provider.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class TrieStorageProviderMock: public TrieStorageProvider {
   public:
    MOCK_METHOD0(setToEphemeral, outcome::result<void>());
    MOCK_METHOD1(setToEphemeralAt, outcome::result<void>(const common::Hash256 &));
    MOCK_METHOD0(setToPersistent, outcome::result<void>());
    MOCK_METHOD1(setToPersistentAt, outcome::result<void>(const common::Hash256 &));
    MOCK_CONST_METHOD0(getCurrentBatch, std::shared_ptr<Batch>());
    MOCK_CONST_METHOD0(tryGetPersistentBatch, boost::optional<std::shared_ptr<PersistentBatch>>());
    MOCK_CONST_METHOD0(isCurrentlyPersistent, bool());

  };

}

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_TRIE_STORAGE_PROVIDER_MOCK
