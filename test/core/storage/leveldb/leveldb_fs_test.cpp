/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/storage/base_fs_test.hpp"

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include "storage/leveldb/leveldb.hpp"
#include "storage/leveldb/leveldb_error.hpp"
#include "testutil/outcome.hpp"

using namespace kagome::storage;
namespace fs = boost::filesystem;

struct LevelDB_Open : public test::BaseFS_Test {
  LevelDB_Open() : test::BaseFS_Test("/tmp/kagome_leveldb_open") {}
};

/**
 * @given options with disabled option `create_if_missing`
 * @when open database
 * @then database can not be opened (since there is no db already)
 */
TEST_F(LevelDB_Open, OpenNonExistingDB) {
  leveldb::Options options;
  options.create_if_missing = false;  // intentionally

  auto r = LevelDB::create(getPathString(), options);
  EXPECT_FALSE(r);
  EXPECT_EQ(r.error(), LevelDBError::INVALID_ARGUMENT);
}

/**
 * @given options with enavble option `create_if_missing`
 * @when open daabase
 * @then database is opened
 */
TEST_F(LevelDB_Open, OpenExistingDB) {
  leveldb::Options options;
  options.create_if_missing = true;  // intentionally

  EXPECT_OUTCOME_TRUE_2(db, LevelDB::create(getPathString(), options));
  EXPECT_TRUE(db) << "db is nullptr";

  boost::filesystem::path p(getPathString());
  EXPECT_TRUE(fs::exists(p));
}
