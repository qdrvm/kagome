/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/storage/base_leveldb_test.hpp"

#include <array>
#include <exception>

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include "storage/leveldb/leveldb.hpp"
#include "storage/database_error.hpp"
#include "testutil/outcome.hpp"

using namespace kagome::storage;
namespace fs = boost::filesystem;

struct LevelDB_Integration_Test : public test::BaseLevelDB_Test {
  LevelDB_Integration_Test()
      : test::BaseLevelDB_Test("/tmp/kagome_leveldb_integration_test") {}

  Buffer key_{1, 3, 3, 7};
  Buffer value_{1, 2, 3};
};

/**
 * @given opened database, with {key}
 * @when read {key}
 * @then {value} is correct
 */
TEST_F(LevelDB_Integration_Test, Put_Get) {
  EXPECT_OUTCOME_TRUE_1(db_->put(key_, value_));
  EXPECT_TRUE(db_->contains(key_));
  EXPECT_OUTCOME_TRUE_2(val, db_->get(key_));
  EXPECT_EQ(val, value_);
}

/**
 * @given empty db
 * @when read {key}
 * @then get "not found"
 */
TEST_F(LevelDB_Integration_Test, Get_NonExistent) {
  EXPECT_FALSE(db_->contains(key_));
  EXPECT_OUTCOME_TRUE_1(db_->remove(key_));
  auto r = db_->get(key_);
  EXPECT_FALSE(r);
  EXPECT_EQ(r.error().value(), (int)DatabaseError::NOT_FOUND);
}

/**
 * @given database with [(i,i) for i in range(6)]
 * @when create batch and write KVs
 * @then data is written only after commit
 */
TEST_F(LevelDB_Integration_Test, WriteBatch) {
  std::list<Buffer> keys{{0}, {1}, {2}, {3}, {4}, {5}};
  Buffer toBeRemoved = {3};
  std::list<Buffer> expected{{0}, {1}, {2}, {4}, {5}};

  auto batch = db_->batch();
  ASSERT_TRUE(batch);

  for (const auto &item : keys) {
    EXPECT_OUTCOME_TRUE_1(batch->put(item, item));
    EXPECT_FALSE(db_->contains(item));
  }
  EXPECT_OUTCOME_TRUE_1(batch->remove(toBeRemoved));
  EXPECT_OUTCOME_TRUE_1(batch->commit());

  for (const auto &item : expected) {
    EXPECT_TRUE(db_->contains(item));
    EXPECT_OUTCOME_TRUE_2(val, db_->get(item));
    EXPECT_EQ(val, item);
  }

  EXPECT_FALSE(db_->contains(toBeRemoved));
}

/**
 * @given database with [(i,i) for i in range(100)]
 * @when iterate over kv pairs forward and backward
 * @then we iterate over all items
 */
TEST_F(LevelDB_Integration_Test, Iterator) {
  const size_t size = 100;
  // 100 buffers of size 1 each; 0..99
  std::list<Buffer> keys;
  for (size_t i = 0; i < size; i++) {
    keys.emplace_back(1, i);
  }

  for (const auto &item : keys) {
    EXPECT_OUTCOME_TRUE_1(db_->put(item, item));
  }

  std::array<size_t, size> counter{};

  logger->warn("forward iteration");
  auto it = db_->cursor();
  EXPECT_OUTCOME_TRUE_1(it->seekToFirst());
  for (; it->isValid(); it->next().assume_value()) {
    auto k = it->key().value();
    auto v = it->value().value();
    EXPECT_EQ(k, v);

    logger->info("key: {}, value: {}", k.toHex(), v.toHex());

    EXPECT_GE(k[0], 0);
    EXPECT_LT(k[0], size);
    EXPECT_GT(k.size(), 0);

    counter[k[0]]++;
  }

  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(counter[i], 1);
  }

  logger->warn("backward iteration");
  size_t c = 0;
  uint8_t index = 0xf;
  Buffer seekTo{index};
  // seek to `index` element
  EXPECT_OUTCOME_TRUE_1(it->seek(seekTo));
  while (it->isValid()) {
    auto k = it->key().value();
    auto v = it->value().value();
    EXPECT_EQ(k, v);

    logger->info("key: {}, value: {}", k.toHex(), v.toHex());

    c++;
    EXPECT_OUTCOME_TRUE_1(it->prev());
  }

  EXPECT_FALSE(it->isValid());
  EXPECT_EQ(c, index + 1);
}
