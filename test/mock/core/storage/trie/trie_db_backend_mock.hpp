/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_DB_BACKEND_MOCK_HPP
#define KAGOME_TRIE_DB_BACKEND_MOCK_HPP

#include <gmock/gmock.h>

#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::trie {

  class TrieDbBackendMock: public TrieDbBackend {
   public:
    MOCK_METHOD0(batch, std::unique_ptr<face::WriteBatch<Buffer, Buffer>>());
    MOCK_METHOD0(cursor, std::unique_ptr<face::MapCursor<Buffer, Buffer>>());
    MOCK_CONST_METHOD1(get, outcome::result<Buffer>(const Buffer &key));
    MOCK_CONST_METHOD1(contains, bool (const Buffer &key));
    MOCK_METHOD2(put, outcome::result<void> (const Buffer &key, const Buffer &value));
    outcome::result<void> put(const common::Buffer &k, common::Buffer &&v) {
      return put_rvalueHack(k, std::move(v));
    }
    MOCK_METHOD2(put_rvalueHack,
                 outcome::result<void>(const common::Buffer &, common::Buffer));
    MOCK_METHOD1(remove, outcome::result<void> (const Buffer &key));
  };

}

#endif  // KAGOME_TRIE_DB_BACKEND_MOCK_HPP
