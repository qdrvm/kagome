/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <memory>

#include "mock/core/storage/persistent_map_mock.hpp"
#include "mock/core/storage/write_batch_mock.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using kagome::storage::face::GenericStorageMock;
using kagome::storage::face::WriteBatchMock;
using kagome::storage::trie::TrieStorageBackendImpl;
using testing::Invoke;
using testing::Return;

static const Buffer kNodePrefix{1};

class TrieDbBackendTest : public testing::Test {
 public:
  std::shared_ptr<GenericStorageMock<Buffer, Buffer>> storage =
      std::make_shared<GenericStorageMock<Buffer, Buffer>>();
  TrieStorageBackendImpl backend{storage, kNodePrefix};
};

/**
 * @given trie backend
 * @when put a value to it
 * @then it puts a prefixed value to the storage
 */
TEST_F(TrieDbBackendTest, Put) {
  Buffer prefixed{kNodePrefix};
  prefixed.put("abc"_buf);
  ((*storage).gmock_put(prefixed, "123"_buf))(
      ::testing::internal::GetWithoutMatchers(), nullptr)
      .InternalExpectedAt(
          "_file_name_", 40, "*storage", "put(prefixed, \"123\"_buf)")
      .WillOnce(Return(outcome::success()));
  EXPECT_OUTCOME_TRUE_1(backend.put("abc"_buf, "123"_buf));
}

/**
 * @given trie backend
 * @when get a value from it
 * @then it takes a prefixed value from the storage
 */
TEST_F(TrieDbBackendTest, Get) {
  Buffer prefixed{kNodePrefix};
  prefixed.put("abc"_buf);
  EXPECT_CALL(*storage, get(prefixed)).WillOnce(Return("123"_buf));
  EXPECT_OUTCOME_TRUE_1(backend.get("abc"_buf));
}

/**
 * @given trie backend batch
 * @when perform operations on it
 * @then it delegates them to the underlying storage batch with added prefixes
 */
TEST_F(TrieDbBackendTest, Batch) {
  auto batch_mock = std::make_unique<WriteBatchMock<Buffer, Buffer>>();
  EXPECT_CALL(*batch_mock, put(Buffer{kNodePrefix}.put("abc"_buf), "123"_buf))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*batch_mock, put(Buffer{kNodePrefix}.put("def"_buf), "123"_buf))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*batch_mock, remove(Buffer{kNodePrefix}.put("abc"_buf)))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*batch_mock, commit()).WillOnce(Return(outcome::success()));

  EXPECT_CALL(*storage, batch())
      .WillOnce(Return(testing::ByMove(std::move(batch_mock))));

  auto batch = backend.batch();
  EXPECT_OUTCOME_TRUE_1(batch->put("abc"_buf, "123"_buf));
  EXPECT_OUTCOME_TRUE_1(batch->put("def"_buf, "123"_buf));
  EXPECT_OUTCOME_TRUE_1(batch->remove("abc"_buf));
  EXPECT_OUTCOME_TRUE_1(batch->commit());
}
