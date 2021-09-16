/**
* Copyright Soramitsu Co., Ltd. All Rights Reserved.
* SPDX-License-Identifier: Apache-2.0
*/

#include <gtest/gtest.h>

#include "runtime/common/module_repository_impl.hpp"

TEST(SmallLruCacheTest, TicksOverflow) {
 auto cache = kagome::runtime::SmallLruCache<int, int, uint8_t> {3};
 (void)cache.put(1, 42);
 (void)cache.put(2, 42);
 (void)cache.put(3, 42);
 for (size_t i = 0; i <= UINT8_MAX + 1; i++) {
   cache.get(2);
 }
 (void)cache.put(4, 42);
 ASSERT_FALSE(cache.get(1));
 ASSERT_TRUE(cache.get(4));
 (void)cache.put(5, 42);
 ASSERT_FALSE(cache.get(3));
 ASSERT_TRUE(cache.get(5));
 ASSERT_TRUE(cache.get(4));
 ASSERT_TRUE(cache.get(2));
}

TEST(SmallLruCacheTest, OldestUsedModulePreempted) {
 auto cache = kagome::runtime::SmallLruCache<int, int> {3};

 (void)cache.put(1, 42);
 (void)cache.put(2, 42);
 (void)cache.put(3, 42);
 ASSERT_TRUE(cache.get(1));
 ASSERT_TRUE(cache.get(1));
 ASSERT_TRUE(cache.get(2));

 (void)cache.put(4, 42);
 (void)cache.put(5, 42);

 ASSERT_FALSE(cache.get(1));
 ASSERT_TRUE(cache.get(2));
 ASSERT_FALSE(cache.get(3));
 ASSERT_TRUE(cache.get(4));
 ASSERT_TRUE(cache.get(5));
}
