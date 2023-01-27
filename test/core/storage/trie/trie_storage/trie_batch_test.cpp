/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "storage/trie/trie_batches.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_rocksdb_test.hpp"

using namespace kagome::storage::trie;
using kagome::api::Session;
using kagome::common::Buffer;
using kagome::common::BufferOrView;
using kagome::common::BufferView;
using kagome::common::Hash256;
using kagome::primitives::BlockHash;
using kagome::storage::Space;
using kagome::storage::trie::StateVersion;
using kagome::subscription::SubscriptionEngine;
using testing::_;
using testing::Invoke;
using testing::Return;

using SessionPtr = std::shared_ptr<Session>;
using SubscriptionEngineType =
    SubscriptionEngine<Buffer, SessionPtr, Buffer, BlockHash>;

class TrieBatchTest : public test::BaseRocksDB_Test {
 public:
  TrieBatchTest() : BaseRocksDB_Test("/tmp/rocksdbtest") {}

  void SetUp() override {
    open();
    auto factory = std::make_shared<PolkadotTrieFactoryImpl>();
    auto codec = std::make_shared<PolkadotCodec>();
    auto serializer = std::make_shared<TrieSerializerImpl>(
        factory,
        codec,
        std::make_shared<TrieStorageBackendImpl>(std::move(db_)));

    empty_hash = serializer->getEmptyRootHash();

    trie =
        TrieStorageImpl::createEmpty(factory, codec, serializer, std::nullopt)
            .value();
  }

  static const std::vector<std::pair<Buffer, Buffer>> data;

  std::unique_ptr<TrieStorage> trie;
  RootHash empty_hash;
};

#define ASSERT_OUTCOME_IS_TRUE(Expression)        \
  {                                               \
    ASSERT_OUTCOME_SUCCESS(result, (Expression)); \
    ASSERT_TRUE(result);                          \
  }
#define ASSERT_OUTCOME_IS_FALSE(Expression)       \
  {                                               \
    ASSERT_OUTCOME_SUCCESS(result, (Expression)); \
    ASSERT_FALSE(result);                         \
  }

const std::vector<std::pair<Buffer, Buffer>> TrieBatchTest::data = {
    {"123456"_hex2buf, "42"_hex2buf},
    {"1234"_hex2buf, "1234"_hex2buf},
    {"010203"_hex2buf, "0a0b"_hex2buf},
    {"010a0b"_hex2buf, "1337"_hex2buf},
    {"0a0b0c"_hex2buf, "deadbeef"_hex2buf}};

void FillSmallTrieWithBatch(TrieBatch &batch) {
  for (auto &entry : TrieBatchTest::data) {
    ASSERT_OUTCOME_SUCCESS_TRY(
        batch.put(entry.first, BufferView{entry.second}));
  }
}

class MockDb : public kagome::storage::InMemoryStorage {
 public:
  MOCK_METHOD(outcome::result<void>, put, (const BufferView &, const Buffer &));
  outcome::result<void> put(const BufferView &k, BufferOrView &&v) override {
    return put(k, v.mut());
  }

  // to retain the ability to call the actual implementation of put from the
  // superclass
  outcome::result<void> true_put(const BufferView &key, const Buffer &value) {
    return InMemoryStorage::put(key, BufferView{value});
  }
};

/**
 * @given an empty trie
 * @when putting some entries into it using a batch
 * @then all inserted entries are accessible from the trie
 */
TEST_F(TrieBatchTest, Put) {
  auto batch = trie->getPersistentBatchAt(empty_hash).value();
  FillSmallTrieWithBatch(*batch);
  // changes are not yet commited
  auto new_batch = trie->getEphemeralBatchAt(empty_hash, {}).value();
  for (auto &entry : data) {
    ASSERT_OUTCOME_ERROR(new_batch->get(entry.first),
                         kagome::storage::trie::TrieError::NO_VALUE);
  }
  ASSERT_OUTCOME_SUCCESS(root_hash, batch->commit(StateVersion::V0));
  // changes are commited
  new_batch = trie->getEphemeralBatchAt(root_hash, {}).value();
  for (auto &entry : data) {
    ASSERT_OUTCOME_SUCCESS(res, new_batch->get(entry.first));
    ASSERT_EQ(res, entry.second);
  }

  ASSERT_OUTCOME_SUCCESS_TRY(
      new_batch->put("102030"_hex2buf, "010203"_hex2buf));
  ASSERT_OUTCOME_SUCCESS_TRY(
      new_batch->put("104050"_hex2buf, "0a0b0c"_hex2buf));
  ASSERT_OUTCOME_SUCCESS(v1, new_batch->get("102030"_hex2buf));
  ASSERT_EQ(v1, "010203"_hex2buf);
  ASSERT_OUTCOME_SUCCESS(v2, new_batch->get("104050"_hex2buf));
  ASSERT_EQ(v2, "0a0b0c"_hex2buf);
}

/**
 * @given a small trie
 * @when removing some entries from it using a batch
 * @then removed entries are no longer in the trie, while the rest of them stays
 */
TEST_F(TrieBatchTest, Remove) {
  auto batch = trie->getPersistentBatchAt(empty_hash).value();
  FillSmallTrieWithBatch(*batch);

  ASSERT_OUTCOME_SUCCESS_TRY(batch->remove(data[2].first));
  ASSERT_OUTCOME_SUCCESS_TRY(batch->remove(data[3].first));
  ASSERT_OUTCOME_SUCCESS_TRY(batch->remove(data[4].first));

  ASSERT_OUTCOME_SUCCESS(root_hash, batch->commit(StateVersion::V0));

  auto read_batch = trie->getEphemeralBatchAt(root_hash, {}).value();
  for (auto i : {2, 3, 4}) {
    ASSERT_OUTCOME_IS_FALSE(read_batch->contains(data[i].first));
  }
  ASSERT_OUTCOME_IS_TRUE(read_batch->contains(data[0].first));
  ASSERT_OUTCOME_IS_TRUE(read_batch->contains(data[1].first));
}

/**
 * @given a small trie
 * @when replacing an entry in it (put a data with an existing key) using a
 * batch
 * @then the value on the key is updated
 */
TEST_F(TrieBatchTest, Replace) {
  auto batch = trie->getPersistentBatchAt(empty_hash).value();
  ASSERT_OUTCOME_SUCCESS_TRY(
      batch->put(data[1].first, BufferView{data[3].second}));
  ASSERT_OUTCOME_SUCCESS(root_hash, batch->commit(StateVersion::V0));
  auto read_batch = trie->getEphemeralBatchAt(root_hash, {}).value();
  ASSERT_OUTCOME_SUCCESS(res, read_batch->get(data[1].first));
  ASSERT_EQ(res, data[3].second);
}

/**
 * @given a trie and its batch
 * @when commiting a batch during which an error occurs
 * @then no changes from the failing batch reach the trie, thus guaranteeing its
 * consistency
 */
TEST_F(TrieBatchTest, ConsistentOnFailure) {
  auto db = std::make_unique<MockDb>();
  /**
   * First time the storage will function correctly, after which it will yield
   * an error
   */
  auto &&expectation = EXPECT_CALL(*db, put(_, _))
                           .Times(1)
                           .WillRepeatedly(Invoke(db.get(), &MockDb::true_put));

  EXPECT_CALL(*db, put(_, _))
      .After(expectation)
      .WillOnce(Return(PolkadotCodec::Error::UNKNOWN_NODE_TYPE));

  auto factory = std::make_shared<PolkadotTrieFactoryImpl>();
  auto codec = std::make_shared<PolkadotCodec>();
  auto serializer = std::make_shared<TrieSerializerImpl>(
      factory, codec, std::make_shared<TrieStorageBackendImpl>(std::move(db)));
  auto trie =
      TrieStorageImpl::createEmpty(factory, codec, serializer, std::nullopt)
          .value();
  auto batch = trie->getPersistentBatchAt(empty_hash).value();

  ASSERT_OUTCOME_SUCCESS_TRY(batch->put("123"_buf, "111"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(batch->commit(StateVersion::V0));

  ASSERT_OUTCOME_SUCCESS_TRY(batch->put("133"_buf, "111"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(batch->put("124"_buf, "111"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(batch->put("154"_buf, "111"_buf));
  ASSERT_FALSE(batch->commit(StateVersion::V0));
}

TEST_F(TrieBatchTest, TopperBatchAtomic) {
  std::shared_ptr<PersistentTrieBatch> p_batch =
      trie->getPersistentBatchAt(empty_hash).value();
  ASSERT_OUTCOME_SUCCESS_TRY(p_batch->put("123"_buf, "abc"_buf));
  ASSERT_OUTCOME_SUCCESS_TRY(p_batch->put("678"_buf, "abc"_buf));

  auto t_batch = p_batch->batchOnTop();

  ASSERT_OUTCOME_SUCCESS_TRY(t_batch->put("123"_buf, "abc"_buf))
  ASSERT_OUTCOME_IS_TRUE(t_batch->contains("123"_buf))
  ASSERT_OUTCOME_SUCCESS_TRY(t_batch->put("345"_buf, "cde"_buf))
  ASSERT_OUTCOME_IS_TRUE(t_batch->contains("345"_buf))
  ASSERT_OUTCOME_SUCCESS_TRY(t_batch->remove("123"_buf))
  ASSERT_OUTCOME_IS_FALSE(t_batch->contains("123"_buf))
  ASSERT_OUTCOME_IS_TRUE(t_batch->contains("678"_buf))

  ASSERT_OUTCOME_IS_FALSE(p_batch->contains("345"_buf))
  ASSERT_OUTCOME_IS_TRUE(p_batch->contains("678"_buf))
  ASSERT_OUTCOME_IS_TRUE(p_batch->contains("123"_buf))

  ASSERT_OUTCOME_SUCCESS_TRY(t_batch->writeBack())

  ASSERT_OUTCOME_IS_TRUE(p_batch->contains("345"_buf))
  ASSERT_OUTCOME_IS_TRUE(p_batch->contains("678"_buf))
  ASSERT_OUTCOME_IS_FALSE(p_batch->contains("123"_buf))
}

/**
 * GIVEN a key present in a persistent batch but not present in its child topper
 * batch WHEN issuing a remove of this key from the topper batch THEN the key
 * must be removed from the persistent batch after a writeback of the topper
 * batch
 */
TEST_F(TrieBatchTest, TopperBatchRemove) {
  std::shared_ptr<PersistentTrieBatch> p_batch =
      trie->getPersistentBatchAt(empty_hash).value();

  ASSERT_OUTCOME_SUCCESS_TRY(p_batch->put("102030"_hex2buf, "010203"_hex2buf));

  auto t_batch = p_batch->batchOnTop();

  t_batch->remove("102030"_hex2buf).value();
  t_batch->writeBack().value();

  ASSERT_FALSE(p_batch->contains("102030"_hex2buf).value());
}

// TODO(Harrm): #595 test clearPrefix
