/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_SERIALIZATION_TRIE_SERIALIZER_MOCK
#define KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_SERIALIZATION_TRIE_SERIALIZER_MOCK

#include <gmock/gmock.h>

#include "storage/trie/serialization/trie_serializer.hpp"

namespace kagome::storage::trie {

  class TrieSerializerMock : public TrieSerializer {
   public:
    MOCK_METHOD(RootHash, getEmptyRootHash, (), (const, override));

    MOCK_METHOD(outcome::result<RootHash>,
                storeTrie,
                (PolkadotTrie &),
                (override));

    MOCK_METHOD(outcome::result<std::shared_ptr<PolkadotTrie>>,
                retrieveTrie,
                (const common::Buffer &),
                (const, override));
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_SERIALIZATION_TRIE_SERIALIZER_MOCK
