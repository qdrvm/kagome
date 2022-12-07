/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_STORAGE_BACKEND_MOCK_HPP
#define KAGOME_TRIE_STORAGE_BACKEND_MOCK_HPP

#include <gmock/gmock.h>

#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::trie {

  class TrieStorageBackendMock : public TrieStorageBackend {
   public:
    MOCK_METHOD(std::unique_ptr<BufferBatch>, batch, (), (override));

    MOCK_METHOD(std::unique_ptr<face::MapCursor<Buffer, Buffer>>,
                cursor,
                (),
                (override));

    MOCK_METHOD(outcome::result<Buffer>,
                get,
                (const Buffer &key),
                (const, override));

    MOCK_METHOD(outcome::result<bool>,
                contains,
                (const Buffer &key),
                (const, override));

    MOCK_METHOD(outcome::result<void>,
                put,
                (const Buffer &key, const Buffer &value),
                (override));
    outcome::result<void> put(const common::Buffer &k,
                              common::Buffer &&v) override {
      return put(k, v);
    }

    MOCK_METHOD(outcome::result<void>, remove, (const Buffer &key), (override));
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TRIE_DB_BACKEND_MOCK_HPP
