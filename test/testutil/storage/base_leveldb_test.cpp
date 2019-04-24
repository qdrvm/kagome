/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "base_leveldb_test.hpp"


namespace test {

  void BaseLevelDB_Test::open() {

    leveldb::Options options;
    options.create_if_missing = true;
    options.info_log = info_log_.get();

    auto r = LevelDB::create(getPathString(), options, logger);
    if (!r) {
      throw std::invalid_argument(r.error().message());
    }

    db = std::move(r.value());
    ASSERT_TRUE(db) << "BaseLevelDB_Test: db is nullptr";
  }

  BaseLevelDB_Test::BaseLevelDB_Test(fs::path path)
      : BaseFS_Test(std::move(path)), info_log_(new LevelDB::Logger(logger)) {
    open();
  }
}  // namespace test
