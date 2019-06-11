/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_STORAGE_TRIE_MOCK_TRIE_DB_HPP
#define KAGOME_TEST_CORE_STORAGE_TRIE_MOCK_TRIE_DB_HPP

#include <gmock/gmock.h>
#include "storage/trie/trie_db.hpp"

namespace kagome::storage::trie {

  class MockTrieDb : public TrieDb {
   public:
    MOCK_CONST_METHOD0(getRootHash, common::Buffer());
    MOCK_METHOD1(clearPrefix, outcome::result<void>(const common::Buffer &));

    MOCK_METHOD2(put,
                 outcome::result<void>(const common::Buffer &,
                                       const common::Buffer &));
    // as GMock doesn't support rvalue-references
    MOCK_METHOD2(put_proxy,
                 outcome::result<void>(const common::Buffer &, common::Buffer));

    outcome::result<void> put(const common::Buffer &key,
                              common::Buffer &&value) override {
      return put_proxy(key, std::move(value));
    }
    MOCK_CONST_METHOD1(get,
                       outcome::result<common::Buffer>(const common::Buffer &));
    MOCK_METHOD1(remove, outcome::result<void>(const common::Buffer &));
    MOCK_CONST_METHOD1(contains, bool(const common::Buffer &));

    MOCK_METHOD0(cursor, std::unique_ptr<BufferMapCursor>());
    MOCK_METHOD0(batch, std::unique_ptr<BufferBatch>());
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TEST_CORE_STORAGE_TRIE_MOCK_TRIE_DB_HPP
