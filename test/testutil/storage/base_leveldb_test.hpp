/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE_LEVELDB_TEST_HPP
#define KAGOME_BASE_LEVELDB_TEST_HPP

#include "storage/leveldb/leveldb.hpp"

#include "testutil/storage/base_fs_test.hpp"

namespace test {

  using kagome::storage::LevelDB;

  struct BaseLevelDB_Test : public BaseFS_Test {
    BaseLevelDB_Test(fs::path path);

    void open();

    std::unique_ptr<LevelDB> db;
  };

}  // namespace test

#endif  // KAGOME_BASE_LEVELDB_TEST_HPP
