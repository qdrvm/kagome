/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "testutil/storage/base_fs_test.hpp"

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include "storage/leveldb/leveldb.hpp"
#include "storage/database_error.hpp"
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
  EXPECT_EQ(r.error(), DatabaseError::INVALID_ARGUMENT);
}

/**
 * @given options with enable option `create_if_missing`
 * @when open database
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
