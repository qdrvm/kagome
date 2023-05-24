/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/lru_cache.hpp"

TEST(SmallLruCacheTest, TicksOverflow) {
  auto cache = kagome::SmallLruCache<int, int, uint8_t>{3};
  cache.put(1, 42);
  cache.put(2, 42);
  cache.put(3, 42);
  for (size_t i = 0; i <= UINT8_MAX + 1; i++) {
    cache.get(2);
  }
  cache.put(4, 42);
  ASSERT_FALSE(cache.get(1).has_value());
  ASSERT_TRUE(cache.get(4).has_value());
  cache.put(5, 42);
  ASSERT_FALSE(cache.get(3).has_value());
  ASSERT_TRUE(cache.get(5).has_value());
  ASSERT_TRUE(cache.get(4).has_value());
  ASSERT_TRUE(cache.get(2).has_value());
}

TEST(SmallLruCacheTest, OldestUsedModulePreempted) {
  auto cache = kagome::SmallLruCache<int, int>{3};

  cache.put(1, 42);
  cache.put(2, 42);
  cache.put(3, 42);
  ASSERT_TRUE(cache.get(1).has_value());
  ASSERT_TRUE(cache.get(1).has_value());
  ASSERT_TRUE(cache.get(2).has_value());

  cache.put(4, 42);
  cache.put(5, 42);

  ASSERT_FALSE(cache.get(1).has_value());
  ASSERT_TRUE(cache.get(2).has_value());
  ASSERT_FALSE(cache.get(3).has_value());
  ASSERT_TRUE(cache.get(4).has_value());
  ASSERT_TRUE(cache.get(5).has_value());
}
