/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie_db/polkadot_trie_batch.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "crypto/hasher/hasher_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_leveldb_test.hpp"
#include "testutil/storage/in_memory_storage.hpp"
#include "testutil/storage/polkadot_trie_db_printer.hpp"

using namespace kagome::storage::trie;
using kagome::common::Buffer;
using kagome::hash::HasherImpl;
using kagome::storage::face::WriteBatch;
using testing::_;
using testing::Invoke;
using testing::Return;

class TrieBatchTest : public test::BaseLevelDB_Test {
 public:
  TrieBatchTest() : BaseLevelDB_Test("/tmp/leveldbtest") {}

  void SetUp() override {
    open();
    trie = std::make_unique<PolkadotTrieDb>(std::move(db_));
  }

  static const std::vector<std::pair<Buffer, Buffer>> data;

  std::unique_ptr<PolkadotTrieDb> trie;
};

const std::vector<std::pair<Buffer, Buffer>> TrieBatchTest::data = {
    {"123456"_hex2buf, "42"_hex2buf},
    {"1234"_hex2buf, "1234"_hex2buf},
    {"010203"_hex2buf, "0a0b"_hex2buf},
    {"010a0b"_hex2buf, "1337"_hex2buf},
    {"0a0b0c"_hex2buf, "deadbeef"_hex2buf}};

void FillSmallTrieWithBatch(WriteBatch<Buffer, Buffer> &batch) {
  for (auto &entry : TrieBatchTest::data) {
    EXPECT_OUTCOME_TRUE_1(batch.put(entry.first, entry.second));
  }
}

class MockPolkadotTrieDb : public PolkadotTrieDb {
 public:
  MockPolkadotTrieDb() : PolkadotTrieDb(nullptr) {}

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
  auto batch = trie->batch();
  FillSmallTrieWithBatch(*batch);

  for (auto &entry : data) {
    EXPECT_OUTCOME_TRUE(res, trie->get(entry.first));
    ASSERT_TRUE(res.empty());
  }
  EXPECT_OUTCOME_TRUE_void(r, batch->commit());
  for (auto &entry : data) {
    EXPECT_OUTCOME_TRUE(res, trie->get(entry.first));
    ASSERT_EQ(res, entry.second);
  }

  EXPECT_OUTCOME_TRUE_1(trie->put("102030"_hex2buf, "010203"_hex2buf));
  EXPECT_OUTCOME_TRUE_1(trie->put("104050"_hex2buf, "0a0b0c"_hex2buf));
  EXPECT_OUTCOME_TRUE(v1, trie->get("102030"_hex2buf));
  ASSERT_EQ(v1, "010203"_hex2buf);
  EXPECT_OUTCOME_TRUE(v2, trie->get("104050"_hex2buf));
  ASSERT_EQ(v2, "0a0b0c"_hex2buf);
}

/**
 * @given a small trie
 * @when removing some entries from it using a batch
 * @then removed entries are no longer in the trie, while the rest of them stays
 */
TEST_F(TrieBatchTest, Remove) {
  auto batch = trie->batch();
  FillSmallTrieWithBatch(*batch);

  EXPECT_OUTCOME_TRUE_1(batch->remove(data[2].first));
  // putting an empty value is removal too
  EXPECT_OUTCOME_TRUE_1(batch->put(data[3].first, Buffer{}));
  EXPECT_OUTCOME_TRUE_1(batch->remove(data[4].first));

  EXPECT_OUTCOME_TRUE_1(batch->commit());

  for (auto i : {2, 3, 4}) {
    ASSERT_FALSE(trie->contains(data[i].first));
  }
  ASSERT_TRUE(trie->contains(data[0].first));
  ASSERT_TRUE(trie->contains(data[1].first));
}

/**
 * @given a small trie
 * @when replacing an entry in it (put a data with an existing key) using a
 * batch
 * @then the value on the key is updated
 */
TEST_F(TrieBatchTest, Replace) {
  auto batch = trie->batch();
  EXPECT_OUTCOME_TRUE_1(batch->put(data[1].first, data[3].second));
  EXPECT_OUTCOME_TRUE_1(batch->commit());
  EXPECT_OUTCOME_TRUE(res, trie->get(data[1].first));
  ASSERT_EQ(res, data[3].second);
}

/**
 * @given a batch with commands
 * @when clearing it
 * @then no batch commands will be executed during commit, as there are none
 * left after clear()
 */
TEST_F(TrieBatchTest, Clear) {
  testing::StrictMock<MockPolkadotTrieDb> mock_trie{};
  PolkadotTrieBatch batch{mock_trie};

  // this method is called when batch tries to apply its actions, which is
  // undesired in this case
  EXPECT_CALL(mock_trie, getRootHash()).Times(0);

  EXPECT_OUTCOME_TRUE_1(batch.put("123"_buf, "111"_buf));
  EXPECT_OUTCOME_TRUE_1(batch.put("133"_buf, "111"_buf));
  EXPECT_OUTCOME_TRUE_1(batch.put("124"_buf, "111"_buf));
  EXPECT_OUTCOME_TRUE_1(batch.remove("123"_buf));
  EXPECT_OUTCOME_TRUE_1(batch.remove("133"_buf));
  batch.clear();
  EXPECT_OUTCOME_TRUE_1(batch.commit());
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

  PolkadotTrieDb trie{std::move(db)};
  PolkadotTrieBatch batch{trie};

  EXPECT_OUTCOME_TRUE_1(batch.put("123"_buf, "111"_buf));
  EXPECT_OUTCOME_TRUE_1(batch.commit());

  auto old_root = trie.getRootHash();
  ASSERT_FALSE(old_root.empty());
  EXPECT_OUTCOME_TRUE_1(batch.put("133"_buf, "111"_buf));
  EXPECT_OUTCOME_TRUE_1(batch.put("124"_buf, "111"_buf));
  EXPECT_OUTCOME_TRUE_1(batch.put("154"_buf, "111"_buf));
  ASSERT_FALSE(batch.commit());
  ASSERT_TRUE(batch.is_empty());

  // if the root hash is unchanged, then the trie content is kept untouched
  // (which we want, as the batch commit failed)
  ASSERT_EQ(trie.getRootHash(), old_root);
}
