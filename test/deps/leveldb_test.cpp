/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <leveldb/db.h>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

void clear(fs::path p) {
  if (fs::exists(p)) {
    fs::remove_all(p);
  }
}

TEST(Leveldb, Integration) {
  leveldb::Options options;
  options.create_if_missing = true;

  fs::path path = "/tmp/leveldb_deps_test";
  clear(path);

  fs::create_directory(path);

  leveldb::DB *db;

  leveldb::DB::Open(options, path.string(), &db);

  leveldb::Status status;

  std::string key = "key";
  std::string value = "value";

  leveldb::WriteOptions wo;

  status = db->Put(wo, key, value);
  ASSERT_TRUE(status.ok());

  std::string read_val;
  status = db->Get(leveldb::ReadOptions(), key, &read_val);
  ASSERT_TRUE(status.ok());

  ASSERT_EQ(read_val, value);

  delete db;

  clear(path);
}
