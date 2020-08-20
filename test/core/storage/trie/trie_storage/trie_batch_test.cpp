/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/impl/persistent_trie_batch_impl.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "storage/trie/trie_batches.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_leveldb_test.hpp"

using namespace kagome::storage::trie;
using kagome::api::Session;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::BlockHash;
using kagome::storage::face::WriteBatch;
using kagome::subscription::SubscriptionEngine;
using testing::_;
using testing::Invoke;
using testing::Return;

using SessionPtr = std::shared_ptr<Session>;
using SubscriptionEngineType =
    SubscriptionEngine<Buffer, SessionPtr, Buffer, BlockHash>;

class TrieBatchTest : public test::BaseLevelDB_Test {
 public:
  TrieBatchTest() : BaseLevelDB_Test("/tmp/leveldbtest") {}

  void SetUp() override {
    open();
    auto factory = std::make_shared<PolkadotTrieFactoryImpl>();
    auto codec = std::make_shared<PolkadotCodec>();
    auto serializer = std::make_shared<TrieSerializerImpl>(
        factory,
        codec,
        std::make_shared<TrieStorageBackendImpl>(std::move(db_), kNodePrefix));

    trie = TrieStorageImpl::createEmpty(factory, codec, serializer, boost::none)
               .value();
  }

  static const std::vector<std::pair<Buffer, Buffer>> data;

  std::unique_ptr<TrieStorage> trie;

  static const Buffer kNodePrefix;
};

const Buffer TrieBatchTest::kNodePrefix{1};

const std::vector<std::pair<Buffer, Buffer>> TrieBatchTest::data = {
    {"123456"_hex2buf, "42"_hex2buf},
    {"1234"_hex2buf, "1234"_hex2buf},
    {"010203"_hex2buf, "0a0b"_hex2buf},
    {"010a0b"_hex2buf, "1337"_hex2buf},
    {"0a0b0c"_hex2buf, "deadbeef"_hex2buf}};

void FillSmallTrieWithBatch(TrieBatch &batch) {
  for (auto &entry : TrieBatchTest::data) {
    EXPECT_OUTCOME_TRUE_1(batch.put(entry.first, entry.second));
  }
}

class MockTrieStorageImpl : public TrieStorageImpl {
 public:
  MockTrieStorageImpl() : TrieStorageImpl({}, nullptr, nullptr, boost::none) {}

  MOCK_CONST_METHOD0(getRootHash, Buffer());
};

class MockDb : public kagome::storage::InMemoryStorage {
 public:
  MOCK_METHOD2(put, outcome::result<void>(const Buffer &, const Buffer &));

  // to retain the ability to call the actual implementation of put from the
  // superclass
  outcome::result<void> true_put(const Buffer &key, const Buffer &value) {
    return InMemoryStorage::put(key, value);
  }
};

/**
 * @given an empty trie
 * @when putting some entries into it using a batch
 * @then all inserted entries are accessible from the trie
 */
TEST_F(TrieBatchTest, Put) {
  auto batch = trie->getPersistentBatch().value();
  FillSmallTrieWithBatch(*batch);
  // changes are not yet commited
  auto new_batch = trie->getEphemeralBatch().value();
  for (auto &entry : data) {
    EXPECT_OUTCOME_FALSE(err, new_batch->get(entry.first));
    ASSERT_EQ(err.value(),
              static_cast<int>(kagome::storage::trie::TrieError::NO_VALUE));
  }
  EXPECT_OUTCOME_TRUE_void(r, batch->commit());
  // changes are commited
  new_batch = trie->getEphemeralBatch().value();
  for (auto &entry : data) {
    EXPECT_OUTCOME_TRUE(res, new_batch->get(entry.first));
    ASSERT_EQ(res, entry.second);
  }

  EXPECT_OUTCOME_TRUE_1(new_batch->put("102030"_hex2buf, "010203"_hex2buf));
  EXPECT_OUTCOME_TRUE_1(new_batch->put("104050"_hex2buf, "0a0b0c"_hex2buf));
  EXPECT_OUTCOME_TRUE(v1, new_batch->get("102030"_hex2buf));
  ASSERT_EQ(v1, "010203"_hex2buf);
  EXPECT_OUTCOME_TRUE(v2, new_batch->get("104050"_hex2buf));
  ASSERT_EQ(v2, "0a0b0c"_hex2buf);
}

/**
 * @given a small trie
 * @when removing some entries from it using a batch
 * @then removed entries are no longer in the trie, while the rest of them stays
 */
TEST_F(TrieBatchTest, Remove) {
  auto batch = trie->getPersistentBatch().value();
  FillSmallTrieWithBatch(*batch);

  EXPECT_OUTCOME_TRUE_1(batch->remove(data[2].first));
  EXPECT_OUTCOME_TRUE_1(batch->remove(data[3].first));
  EXPECT_OUTCOME_TRUE_1(batch->remove(data[4].first));

  EXPECT_OUTCOME_TRUE_1(batch->commit());

  auto read_batch = trie->getEphemeralBatch().value();
  for (auto i : {2, 3, 4}) {
    ASSERT_FALSE(read_batch->contains(data[i].first));
  }
  ASSERT_TRUE(read_batch->contains(data[0].first));
  ASSERT_TRUE(read_batch->contains(data[1].first));
}

/**
 * @given a small trie
 * @when replacing an entry in it (put a data with an existing key) using a
 * batch
 * @then the value on the key is updated
 */
TEST_F(TrieBatchTest, Replace) {
  auto batch = trie->getPersistentBatch().value();
  EXPECT_OUTCOME_TRUE_1(batch->put(data[1].first, data[3].second));
  EXPECT_OUTCOME_TRUE_1(batch->commit());
  auto read_batch = trie->getEphemeralBatch().value();
  EXPECT_OUTCOME_TRUE(res, read_batch->get(data[1].first));
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
   * Five times the storage will function correctly, after which it will yield
   * an error
   */
  auto &&expectation = EXPECT_CALL(*db, put(_, _))
                           .Times(5)
                           .WillRepeatedly(Invoke(db.get(), &MockDb::true_put));

  EXPECT_CALL(*db, put(_, _))
      .After(expectation)
      .WillOnce(Return(PolkadotCodec::Error::UNKNOWN_NODE_TYPE));

  auto factory = std::make_shared<PolkadotTrieFactoryImpl>();
  auto codec = std::make_shared<PolkadotCodec>();
  auto serializer = std::make_shared<TrieSerializerImpl>(
      factory,
      codec,
      std::make_shared<TrieStorageBackendImpl>(std::move(db), kNodePrefix));
  auto trie =
      TrieStorageImpl::createEmpty(factory, codec, serializer, boost::none)
          .value();
  auto batch = trie->getPersistentBatch().value();

  EXPECT_OUTCOME_TRUE_1(batch->put("123"_buf, "111"_buf));
  EXPECT_OUTCOME_TRUE_1(batch->commit());

  auto old_root = trie->getRootHash();
  ASSERT_FALSE(old_root.empty());
  EXPECT_OUTCOME_TRUE_1(batch->put("133"_buf, "111"_buf));
  EXPECT_OUTCOME_TRUE_1(batch->put("124"_buf, "111"_buf));
  EXPECT_OUTCOME_TRUE_1(batch->put("154"_buf, "111"_buf));
  ASSERT_FALSE(batch->commit());

  // if the root hash is unchanged, then the trie content is kept untouched
  // (which we want, as the batch commit failed)
  ASSERT_EQ(trie->getRootHash(), old_root);
}

TEST_F(TrieBatchTest, TopperBatchAtomic) {
  std::shared_ptr<PersistentTrieBatch> p_batch =
      trie->getPersistentBatch().value();
  EXPECT_OUTCOME_TRUE_1(p_batch->put("123"_buf, "abc"_buf));
  EXPECT_OUTCOME_TRUE_1(p_batch->put("678"_buf, "abc"_buf));

  auto t_batch = p_batch->batchOnTop();

  EXPECT_OUTCOME_TRUE_1(t_batch->put("123"_buf, "abc"_buf));
  ASSERT_TRUE(t_batch->contains("123"_buf));
  EXPECT_OUTCOME_TRUE_1(t_batch->put("345"_buf, "cde"_buf));
  ASSERT_TRUE(t_batch->contains("345"_buf));
  EXPECT_OUTCOME_TRUE_1(t_batch->remove("123"_buf));
  ASSERT_FALSE(t_batch->contains("123"_buf));
  ASSERT_TRUE(t_batch->contains("678"_buf));

  ASSERT_FALSE(p_batch->contains("345"_buf));
  ASSERT_TRUE(p_batch->contains("678"_buf));
  ASSERT_TRUE(p_batch->contains("123"_buf));

  EXPECT_OUTCOME_TRUE_1(t_batch->writeBack());

  ASSERT_TRUE(p_batch->contains("345"_buf));
  ASSERT_TRUE(p_batch->contains("678"_buf));
  ASSERT_FALSE(p_batch->contains("123"_buf));
}
