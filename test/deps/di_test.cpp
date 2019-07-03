/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <boost/di.hpp>

class ctor {
 public:
  explicit ctor(int i) : i(i) {}
  int i;
};

struct aggregate {
  double d;
};

class example {
 public:
  example(aggregate a, const ctor &c) {
    EXPECT_EQ(87.0, a.d);
    EXPECT_EQ(42, c.i);
  };
};

/**
 * @brief If test compiles, then DI works.
 */
TEST(Boost, DI) {
  using namespace boost;

  // clang-format off
  const auto injector = di::make_injector(
    di::bind<int>.to(42),
    di::bind<double>.to(87.0)
  );
  // clang-format on

  injector.create<example>();
}
