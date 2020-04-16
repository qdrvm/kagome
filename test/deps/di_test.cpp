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
  virtual ~example() = default;

  example(aggregate a, const ctor &c) {
    EXPECT_EQ(87.0, a.d);
    EXPECT_EQ(42, c.i);
  };

  virtual void func() {}
};

struct Derived : public example {
  ~Derived() override = default;

  void func() override {}
};

template <typename... T>
auto useBind(){
  return boost::di::bind<example*[]>.template to<T...>();
}

/**
 * @brief If test compiles, then DI works.
 */
TEST(Boost, DI) {
  using namespace boost;

  // clang-format off
  const auto injector = di::make_injector(
    di::bind<int>.to(42)[boost::di::override],
    di::bind<double>.to(87.0),
    useBind<Derived>()
  );
  // clang-format on

  auto a = injector.create<example>();
  auto b = injector.create<std::shared_ptr<example>>();
  auto c = injector.create<std::unique_ptr<example>>();

  (void)a;
  (void)b;
  (void)c;
}
