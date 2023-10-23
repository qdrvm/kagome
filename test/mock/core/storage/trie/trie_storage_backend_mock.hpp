/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::trie {

  class TrieStorageBackendMock : public TrieNodeStorageBackend,
                                 public TrieValueStorageBackend {
   public:
    MOCK_METHOD(std::unique_ptr<BufferBatch>, batch, (), (override));

    MOCK_METHOD((std::unique_ptr<face::MapCursor<Buffer, Buffer>>),
                cursor,
                (),
                (override));

    MOCK_METHOD(outcome::result<BufferOrView>,
                get,
                (const BufferView &key),
                (const, override));

    MOCK_METHOD(outcome::result<std::optional<BufferOrView>>,
                tryGet,
                (const BufferView &key),
                (const, override));

    MOCK_METHOD(outcome::result<bool>,
                contains,
                (const BufferView &key),
                (const, override));

    MOCK_METHOD(bool, empty, (), (const, override));

    MOCK_METHOD(outcome::result<void>,
                put,
                (const BufferView &key, BufferOrView &&value),
                (override));

    MOCK_METHOD(outcome::result<void>,
                remove,
                (const BufferView &key),
                (override));
  };

}  // namespace kagome::storage::trie
