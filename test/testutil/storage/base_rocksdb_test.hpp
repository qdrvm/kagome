/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE_ROCKSDB_TEST_HPP
#define KAGOME_BASE_ROCKSDB_TEST_HPP

#include "testutil/storage/base_fs_test.hpp"

#include "storage/rocksdb/rocksdb.hpp"

namespace test {

  struct BaseRocksDB_Test : public BaseFS_Test {
    using RocksDB = kagome::storage::RocksDb;

    BaseRocksDB_Test(fs::path path);

    void open();

    void SetUp() override;

    void TearDown() override;

    std::shared_ptr<RocksDB> rocks_;
    std::shared_ptr<kagome::storage::BufferStorage> db_;
  };

}  // namespace test

#endif  // KAGOME_BASE_ROCKSDB_TEST_HPP
