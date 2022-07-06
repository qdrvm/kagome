/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <rocksdb/slice.h>

TEST(RocksDB, DependencyInitialized) {
  rocksdb::Slice slice;
  std::string str = slice.ToString();
  ASSERT_EQ(0, static_cast<int>(str.size()));
}
