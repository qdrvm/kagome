/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/polkadot_trie_db.hpp"

#include <memory>

#include <gtest/gtest.h>
#include "mock/core/storage/persistent_map_mock.hpp"
#include "mock/core/storage/write_batch_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using kagome::storage::face::PersistentMapMock;
using kagome::storage::face::WriteBatchMock;
using kagome::storage::trie::PolkadotTrieDb;
using kagome::storage::trie::PolkadotTrieDb;
using testing::Invoke;
using testing::Return;

static const Buffer kNodePrefix{1};
static const Buffer kRootHashKey{0};

class TrieDbBackendTest : public testing::Test {
 public:
  std::shared_ptr<PersistentMapMock<Buffer, Buffer>> storage =
      std::make_shared<PersistentMapMock<Buffer, Buffer>>();
  PersistentTrieDbBackend backend{storage, kNodePrefix, kRootHashKey};
};

/**
 * @given trie backend
 * @when put a value to it
 * @then it puts a prefixed value to the storage
 */
TEST_F(TrieDbBackendTest, Put) {
  Buffer prefixed{kNodePrefix};
  prefixed.put("abc"_buf);
  EXPECT_CALL(*storage, put_rvalueHack(prefixed, "123"_buf))
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
  EXPECT_CALL(*batch_mock,
              put_rvalue(Buffer{kNodePrefix}.put("abc"_buf), "123"_buf))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*batch_mock,
              put_rvalue(Buffer{kNodePrefix}.put("def"_buf), "123"_buf))
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

/**
 * @given trie backend
 * @when saving and fetching the root hash
 * @then it is saved by the key specified during construction of backend and is
 * correctly fetched
 */
TEST_F(TrieDbBackendTest, Root) {
  Buffer root_hash{"12345"_buf};
  EXPECT_CALL(*storage, put(kRootHashKey, root_hash))
      .WillOnce(Return(outcome::success()));
  EXPECT_OUTCOME_TRUE_1(backend.saveRootHash(root_hash));
  EXPECT_CALL(*storage, get(kRootHashKey)).WillOnce(Return(root_hash));
  EXPECT_OUTCOME_TRUE(hash, backend.getRootHash());
  ASSERT_EQ(hash, root_hash);
}
