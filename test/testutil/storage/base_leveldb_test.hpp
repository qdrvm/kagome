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

#ifndef KAGOME_BASE_LEVELDB_TEST_HPP
#define KAGOME_BASE_LEVELDB_TEST_HPP

#include "testutil/storage/base_fs_test.hpp"

#include "storage/leveldb/leveldb.hpp"

namespace test {

  struct BaseLevelDB_Test : public BaseFS_Test {
    using LevelDB = kagome::storage::LevelDB;

    BaseLevelDB_Test(fs::path path);

    void open();

    void SetUp() override;

    void TearDown() override;

    std::shared_ptr<LevelDB> db_;
  };

}  // namespace test

#endif  // KAGOME_BASE_LEVELDB_TEST_HPP
