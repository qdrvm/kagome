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

#include "testutil/storage/base_leveldb_test.hpp"

namespace test {

  void BaseLevelDB_Test::open() {
    leveldb::Options options;
    options.create_if_missing = true;

    auto r = LevelDB::create(getPathString(), options);
    if (!r) {
      throw std::invalid_argument(r.error().message());
    }

    db_ = std::move(r.value());
    ASSERT_TRUE(db_) << "BaseLevelDB_Test: db is nullptr";
  }

  BaseLevelDB_Test::BaseLevelDB_Test(fs::path path)
      : BaseFS_Test(std::move(path)) {}

  void BaseLevelDB_Test::SetUp() {
    open();
  }

  void BaseLevelDB_Test::TearDown() {
    clear();
  }
}  // namespace test
