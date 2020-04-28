/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_DB_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_DB_MOCK_HPP

#include <gmock/gmock.h>

#include "storage/trie/trie_db.hpp"

namespace kagome::storage::trie {

  class TrieDbMock : public TrieDb {
   public:
    MOCK_METHOD0(
        batch,
        std::unique_ptr<face::WriteBatch<common::Buffer, common::Buffer>>());

    MOCK_METHOD1(resetState, void(const common::Buffer &));

    MOCK_METHOD0(
        cursor,
        std::unique_ptr<face::MapCursor<common::Buffer, common::Buffer>>());

    MOCK_CONST_METHOD1(get,
                       outcome::result<common::Buffer>(const common::Buffer &));

    MOCK_CONST_METHOD1(contains, bool(const common::Buffer &));

    MOCK_METHOD2(put,
                 outcome::result<void>(const common::Buffer &,
                                       const common::Buffer &));

    outcome::result<void> put(const common::Buffer &k, common::Buffer &&v) {
      return put_rvalueHack(k, std::move(v));
    }
    MOCK_METHOD2(put_rvalueHack,
                 outcome::result<void>(const common::Buffer &, common::Buffer));

    MOCK_METHOD1(remove, outcome::result<void>(const common::Buffer &));

    MOCK_CONST_METHOD0(getRootHash, common::Buffer());

    MOCK_METHOD1(clearPrefix, outcome::result<void>(const common::Buffer &buf));

    MOCK_CONST_METHOD0(empty, bool());
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_DB_MOCK_HPP
