/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include "storage/leveldb/leveldb.hpp"
#include "storage/leveldb/leveldb_error.hpp"
#include "testutil/outcome.hpp"

#include "testutil/storage/base_fs_test.hpp"

using namespace kagome::storage;
namespace fs = boost::filesystem;

struct LevelDB_Open : public test::BaseFS_Test {
  LevelDB_Open() : test::BaseFS_Test("/tmp/kagome_leveldb_open") {}
};

TEST_F(LevelDB_Open, OpenNonExistingDB) {
  leveldb::Options options;
  options.create_if_missing = false;  // intentionally

  auto r = LevelDB::create(getPathString(), options);
  EXPECT_FALSE(r);
  EXPECT_EQ(r.error(), LevelDBError::kInvalidArgument);
}

TEST_F(LevelDB_Open, OpenExistingDB) {
  leveldb::Options options;
  options.create_if_missing = true;  // intentionally

  EXPECT_OUTCOME_TRUE_2(db, LevelDB::create(getPathString(), options));
  EXPECT_TRUE(db) << "db is nullptr";

  boost::filesystem::path p(getPathString());
  EXPECT_TRUE(fs::exists(p));
}
