/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_BATCHES_MOCK
#define KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_BATCHES_MOCK

#include <gmock/gmock.h>

#include "storage/trie/trie_batches.hpp"

namespace kagome::storage::trie {

  class TrieBatchMock : public TrieBatch {
   public:
    MOCK_METHOD(outcome::result<Buffer>,
                getMock,
                (const common::BufferView &),
                (const));

    MOCK_METHOD(outcome::result<std::optional<Buffer>>,
                tryGetMock,
                (const common::BufferView &),
                (const));

    outcome::result<BufferOrView> get(const BufferView &key) const override {
      OUTCOME_TRY(value, getMock(key));
      return value;
    }

    outcome::result<std::optional<BufferOrView>> tryGet(
        const BufferView &key) const override {
      OUTCOME_TRY(value, tryGetMock(key));
      if (value) {
        return std::move(*value);
      }
      return std::nullopt;
    }

    MOCK_METHOD(std::unique_ptr<PolkadotTrieCursor>,
                trieCursor,
                (),
                (override));

    MOCK_METHOD(outcome::result<bool>,
                contains,
                (const common::BufferView &),
                (const, override));

    MOCK_METHOD(outcome::result<void>,
                put,
                (const common::BufferView &, const Buffer &));
    outcome::result<void> put(const common::BufferView &k,
                              BufferOrView &&v) override {
      return put(k, v.mut());
    }

    MOCK_METHOD(outcome::result<void>,
                remove,
                (const common::BufferView &),
                (override));

    MOCK_METHOD((outcome::result<std::tuple<bool, uint32_t>>),
                clearPrefix,
                (const common::BufferView &buf, std::optional<uint64_t> limit),
                (override));

    MOCK_METHOD(bool, empty, (), (const, override));

    MOCK_METHOD(outcome::result<storage::trie::RootHash>,
                commit,
                (StateVersion),
                (override));

    MOCK_METHOD(outcome::result<std::optional<std::shared_ptr<TrieBatch>>>,
                createChildBatch,
                (common::BufferView path),
                (override));
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_BATCHES_MOCK
