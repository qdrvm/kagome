/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <deque>
#include <map>
#include <optional>
#include <scale/outcome/outcome_throw.hpp>
#include <scale/types.hpp>
#include <tuple>
#include <type_traits>
#include <vector>

#define REP0(X)
#define REP1(X) X
#define REP2(X) REP1(X), X
#define REP3(X) REP2(X), X
#define REP4(X) REP3(X), X
#define REP5(X) REP4(X), X
#define REP6(X) REP5(X), X
#define REP7(X) REP6(X), X
#define REP8(X) REP7(X), X
#define REP9(X) REP8(X), X
#define REP10(X) REP9(X), X

#define REP0Y(X)
#define REP1Y(X) X##1
#define REP2Y(X) REP1Y(X), X##2
#define REP3Y(X) REP2Y(X), X##3
#define REP4Y(X) REP3Y(X), X##4
#define REP5Y(X) REP4Y(X), X##5
#define REP6Y(X) REP5Y(X), X##6
#define REP7Y(X) REP6Y(X), X##7
#define REP8Y(X) REP7Y(X), X##8
#define REP9Y(X) REP8Y(X), X##9

#define REP0Y_REF(X)
#define REP1Y_REF(X) std::cref(X##1)
#define REP2Y_REF(X) REP1Y_REF(X), std::cref(X##2)
#define REP3Y_REF(X) REP2Y_REF(X), std::cref(X##3)
#define REP4Y_REF(X) REP3Y_REF(X), std::cref(X##4)
#define REP5Y_REF(X) REP4Y_REF(X), std::cref(X##5)
#define REP6Y_REF(X) REP5Y_REF(X), std::cref(X##6)
#define REP7Y_REF(X) REP6Y_REF(X), std::cref(X##7)
#define REP8Y_REF(X) REP7Y_REF(X), std::cref(X##8)
#define REP9Y_REF(X) REP8Y_REF(X), std::cref(X##9)

#define REPEAT(TENS, ONES, X) REP##TENS(REP10(X)) REP##ONES(X)
#define REPEATY(ONES, X) REP##ONES##Y(X)
#define REPEATY_REF(ONES, X) REP##ONES##Y_REF(X)

#define TO_TUPLE_N(ONES)                                                      \
  if constexpr (is_braces_constructible<type, REPEAT(0, ONES, any_type)>{}) { \
    const auto &[REPEATY(ONES, p)] = object;                                  \
    return std::make_tuple(REPEATY_REF(ONES, p));                             \
  }

#define TO_TUPLE1 \
  TO_TUPLE_N(1) else { return std::make_tuple(); }
#define TO_TUPLE2 TO_TUPLE_N(2) else TO_TUPLE1
#define TO_TUPLE3 TO_TUPLE_N(3) else TO_TUPLE2
#define TO_TUPLE4 TO_TUPLE_N(4) else TO_TUPLE3
#define TO_TUPLE5 TO_TUPLE_N(5) else TO_TUPLE4
#define TO_TUPLE6 TO_TUPLE_N(6) else TO_TUPLE5
#define TO_TUPLE7 TO_TUPLE_N(7) else TO_TUPLE6
#define TO_TUPLE8 TO_TUPLE_N(8) else TO_TUPLE7
#define TO_TUPLE9 TO_TUPLE_N(9) else TO_TUPLE8

namespace kagome::utils {

  template <typename T, typename... TArgs>
  decltype(void(T{{std::declval<TArgs>()}...}), std::true_type{})
  test_is_braces_constructible(int);

  template <typename, typename...>
  std::false_type test_is_braces_constructible(...);

  template <typename T, typename... TArgs>
  using is_braces_constructible =
      decltype(test_is_braces_constructible<T, TArgs...>(0));

  struct any_type {
    template <typename T>
    constexpr operator T();  // non explicit
  };

  template <typename T>
  inline auto to_tuple_refs(const T &object) {
    using type = std::decay_t<T>;
    TO_TUPLE9;
  }
}  // namespace kagome::utils

#undef REP0
#undef REP1
#undef REP2
#undef REP3
#undef REP4
#undef REP5
#undef REP6
#undef REP7
#undef REP8
#undef REP9
#undef REP10
#undef REP0Y
#undef REP1Y
#undef REP2Y
#undef REP3Y
#undef REP4Y
#undef REP5Y
#undef REP6Y
#undef REP7Y
#undef REP8Y
#undef REP9Y
#undef REP0Y_REF
#undef REP1Y_REF
#undef REP2Y_REF
#undef REP3Y_REF
#undef REP4Y_REF
#undef REP5Y_REF
#undef REP6Y_REF
#undef REP7Y_REF
#undef REP8Y_REF
#undef REP9Y_REF
#undef REPEAT
#undef REPEATY
#undef REPEATY_REF
#undef TO_TUPLE_N
#undef TO_TUPLE1
#undef TO_TUPLE2
#undef TO_TUPLE3
#undef TO_TUPLE4
#undef TO_TUPLE5
#undef TO_TUPLE6
#undef TO_TUPLE7
#undef TO_TUPLE8
#undef TO_TUPLE9
