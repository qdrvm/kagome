/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE_ROCKSDB_TEST_HPP
#define KAGOME_BASE_ROCKSDB_TEST_HPP

#include "testutil/storage/base_fs_test.hpp"

#include "storage/rocksdb/rocksdb.hpp"

namespace test {

  struct BaseRocksDB_Test : public BaseFS_Test {
    using RocksDB = kagome::storage::RocksDB;

    BaseRocksDB_Test(fs::path path);

    void open();

    void SetUp() override;

    void TearDown() override;

    std::shared_ptr<RocksDB> db_;
  };

}  // namespace test

#endif  // KAGOME_BASE_ROCKSDB_TEST_HPP
