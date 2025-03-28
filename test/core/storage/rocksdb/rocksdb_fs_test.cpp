/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/storage/base_fs_test.hpp"

#include <gtest/gtest.h>

#include <qtils/test/outcome.hpp>

#include "filesystem/common.hpp"
#include "storage/database_error.hpp"
#include "storage/rocksdb/rocksdb.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome::storage;
namespace fs = kagome::filesystem;

struct RocksDb_Open : public test::BaseFS_Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  RocksDb_Open() : test::BaseFS_Test("/tmp/kagome_rocksdb_open") {}
};

/**
 * @given options with disabled option `create_if_missing`
 * @when open database
 * @then database can not be opened (since there is no db already)
 */
TEST_F(RocksDb_Open, OpenNonExistingDB) {
  rocksdb::Options options;
  options.create_if_missing = false;  // intentionally

  auto r = RocksDb::create(getPathString(), options);
  EXPECT_FALSE(r);
  EXPECT_EQ(r.error(), DatabaseError::INVALID_ARGUMENT);
}

/**
 * @given options with enable option `create_if_missing`
 * @when open database
 * @then database is opened
 */
TEST_F(RocksDb_Open, OpenExistingDB) {
  rocksdb::Options options;
  options.create_if_missing = true;  // intentionally

  ASSERT_OUTCOME_SUCCESS(db, RocksDb::create(getPathString(), options));
  EXPECT_TRUE(db) << "db is nullptr";

  kagome::filesystem::path p(getPathString());
  EXPECT_TRUE(fs::exists(p));
}
