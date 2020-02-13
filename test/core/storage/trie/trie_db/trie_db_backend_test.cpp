/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <memory>

#include "mock/core/storage/persistent_map_mock.hpp"
#include "mock/core/storage/write_batch_mock.hpp"
#include "storage/trie/impl/persistent_trie_db_backend.hpp"
#include "storage/trie/impl/readonly_trie_db_backend.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::storage::face::PersistentMapMock;
using kagome::storage::face::WriteBatchMock;
using kagome::storage::trie::PersistentTrieDbBackend;
using kagome::storage::trie::ReadonlyTrieDbBackend;
using kagome::storage::trie::TrieDbBackend;
using testing::Invoke;
using testing::Return;

static const Buffer kNodePrefix{1};
static const Hash256 kRootHashKey{};

template <typename Backend>
class TrieDbBackendTest : public testing::Test {
 public:
  void SetUp() override {
    backend = std::make_unique<Backend>(storage, kRootHashKey, kNodePrefix);
  }

  std::shared_ptr<PersistentMapMock<Buffer, Buffer>> storage =
      std::make_shared<PersistentMapMock<Buffer, Buffer>>();
  std::unique_ptr<TrieDbBackend> backend;
};

using Types = testing::Types<PersistentTrieDbBackend, ReadonlyTrieDbBackend>;
TYPED_TEST_CASE(TrieDbBackendTest, Types);

/**
 * @given trie backend
 * @when put a value to it
 * @then it puts a prefixed value to the storage
 */
TYPED_TEST(TrieDbBackendTest, Put) {
  Buffer prefixed{kNodePrefix};
  prefixed.put("abc"_buf);
  if constexpr (std::is_same_v<TypeParam, PersistentTrieDbBackend>) {
    EXPECT_CALL(*this->storage, put_rvalueHack(prefixed, "123"_buf))
        .WillOnce(Return(outcome::success()));
    EXPECT_OUTCOME_TRUE_1(this->backend->put("abc"_buf, "123"_buf));
  } else if constexpr (std::is_same_v<TypeParam, ReadonlyTrieDbBackend>) {
    EXPECT_OUTCOME_FALSE(e, this->backend->put("abc"_buf, "123"_buf));
    ASSERT_EQ(e, ReadonlyTrieDbBackend::Error::WRITE_TO_READONLY_TRIE);
  }
}

/**
 * @given trie backend
 * @when get a value from it
 * @then it takes a prefixed value from the storage
 */
TYPED_TEST(TrieDbBackendTest, Get) {
  Buffer prefixed{kNodePrefix};
  prefixed.put("abc"_buf);
  EXPECT_CALL(*this->storage, get(prefixed)).WillOnce(Return("123"_buf));
  EXPECT_OUTCOME_TRUE_1(this->backend->get("abc"_buf));
}

/**
 * @given trie backend batch
 * @when perform operations on it
 * @then it delegates them to the underlying storage batch with added prefixes
 */
TYPED_TEST(TrieDbBackendTest, Batch) {
  if constexpr (std::is_same_v<TypeParam, PersistentTrieDbBackend>) {
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

    EXPECT_CALL(*this->storage, batch())
        .WillOnce(Return(testing::ByMove(std::move(batch_mock))));

    auto batch = this->backend->batch();
    EXPECT_OUTCOME_TRUE_1(batch->put("abc"_buf, "123"_buf));
    EXPECT_OUTCOME_TRUE_1(batch->put("def"_buf, "123"_buf));
    EXPECT_OUTCOME_TRUE_1(batch->remove("abc"_buf));
    EXPECT_OUTCOME_TRUE_1(batch->commit());
  } else if constexpr (std::is_same_v<TypeParam, ReadonlyTrieDbBackend>) {
    ASSERT_EQ(this->backend->batch(), nullptr);
  }
}

/**
 * @given trie backend
 * @when saving and fetching the root hash
 * @then it is saved by the key specified during construction of backend and is
 * correctly fetched
 */
TYPED_TEST(TrieDbBackendTest, Root) {
  Buffer root_hash{"12345"_buf};
  if constexpr (std::is_same_v<TypeParam, PersistentTrieDbBackend>) {
    EXPECT_CALL(*this->storage, put(Buffer{kRootHashKey}, root_hash))
        .WillOnce(Return(outcome::success()));
    EXPECT_OUTCOME_TRUE_1(this->backend->saveRootHash(root_hash));
    EXPECT_CALL(*this->storage, get(Buffer{kRootHashKey})).WillOnce(Return(root_hash));
    EXPECT_OUTCOME_TRUE(hash, this->backend->getRootHash());
    ASSERT_EQ(hash, root_hash);
  } else if constexpr (std::is_same_v<TypeParam, ReadonlyTrieDbBackend>) {
    EXPECT_OUTCOME_TRUE(hash, this->backend->getRootHash());
    ASSERT_EQ(hash, kRootHashKey);

    EXPECT_OUTCOME_FALSE(e, this->backend->saveRootHash(root_hash));
    ASSERT_EQ(e, ReadonlyTrieDbBackend::CHANGE_ROOT_OF_READONLY_TRIE);
  }
}
