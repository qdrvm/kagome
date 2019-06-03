/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_STORAGE_MERKLE_MOCK_TRIE_DB_HPP_
#define KAGOME_TEST_CORE_STORAGE_MERKLE_MOCK_TRIE_DB_HPP_

#include <gmock/gmock.h>
#include "storage/merkle/trie_db_test.hpp"

namespace kagome::storage::merkle {

  class MockTrieDb : public TrieDb {
   public:
    MOCK_CONST_METHOD0(getRoot, common::Buffer());
    MOCK_METHOD1(clearPrefix, void(const common::Buffer &));

    MOCK_METHOD2(put,
                 outcome::result<void>(const common::Buffer &,
                                       const common::Buffer &));
    MOCK_CONST_METHOD1(get,
                       outcome::result<common::Buffer>(const common::Buffer &));
    MOCK_METHOD1(remove, outcome::result<void>(
        const common::Buffer &));
    MOCK_CONST_METHOD1(contains, bool(const common::Buffer &));

    MOCK_METHOD0(cursor, std::unique_ptr<BufferMapCursor>());
    MOCK_METHOD0(batch, std::unique_ptr<BufferBatch>());
  };

}  // namespace kagome::storage::merkle

#endif  // KAGOME_TEST_CORE_STORAGE_MERKLE_MOCK_TRIE_DB_HPP_
