/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STRUCT_TO_TUPLE_HPP
#define KAGOME_STRUCT_TO_TUPLE_HPP

#include <deque>
#include <map>
#include <optional>
#include <tuple>
#include <type_traits>
#include <vector>
#include <scale/types.hpp>

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
    const auto &[REPEATY(ONES, p)] = object;                                       \
    return std::make_tuple(REPEATY_REF(ONES, p));                                 \
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
  decltype(void(T{std::declval<TArgs>()...}), std::true_type{})
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
  auto to_tuple(const T &object) noexcept {
    using type = std::decay_t<T>;
    TO_TUPLE9;
  }
}  // namespace kagome::utils

namespace kagome::scale {

  template <typename T>
  constexpr void encode(const T &v);

  template <typename... Ts>
  constexpr void encode(const std::tuple<Ts...> &v);

  template <typename T, typename... Args>
  constexpr void encode(const T &t, const Args &...args);

  template <typename T>
  constexpr void encode(const std::vector<T> &c);

  template<>
  constexpr void encode(const ::scale::CompactInteger &value);

  constexpr void putByte(const uint8_t *const val, size_t count) {
    std::cout << count << std::endl;
    for (size_t i = 0; i < count; ++i) {
      std::cout << std::hex << (uint32_t)val[i];
    }
    std::cout << std::endl;
  }

  template<>
  constexpr void encode(const ::scale::CompactInteger &value) {
      if (value < 0) {
        raise(EncodeError::NEGATIVE_COMPACT_INTEGER);
      }

      if (value < compact::EncodingCategoryLimits::kMinUint16) {
        encodeFirstCategory(value.convert_to<uint8_t>(), out);
        return;
      }

      if (value < compact::EncodingCategoryLimits::kMinUint32) {
        encodeSecondCategory(value.convert_to<uint16_t>(), out);
        return;
      }

      if (value < compact::EncodingCategoryLimits::kMinBigInteger) {
        encodeThirdCategory(value.convert_to<uint32_t>(), out);
        return;
      }
  }

  template <
      typename It,
      typename = std::enable_if_t<
          !std::is_same_v<typename std::iterator_traits<It>::value_type, void>>>
  constexpr void encode(It begin, It end) {
    while (begin != end) {
      encode(*begin);
      ++begin;
    }
  }

  template <typename T>
  constexpr void encode(const std::vector<T> &c) {
    encode(::scale::CompactInteger{c.size()});
    encode(c.begin(), c.end());
  }

  template <typename... Ts>
  constexpr void encode(const std::tuple<Ts...> &v) {
    if constexpr (sizeof...(Ts) > 0) {
      std::apply(
          [&](const auto &...s) { (..., encode(s)); },
          v);
    }
  }

  template <typename T>
  constexpr void encode(const T &v) {
    using I = std::decay_t<T>;
    if constexpr (std::is_integral_v<I>) {
      if constexpr (std::is_same_v<I, bool>) {
        const uint8_t byte = (v ? 1u : 0u);
        putByte(&byte, 1ul);
        return;
      }

      if constexpr (sizeof(I) == 1u) {
        putByte(&v, 1ul);
        return;
      }

      constexpr size_t size = sizeof(I);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
      if constexpr (size == 8) {
        v = __builtin_bswap64(v);
      } else if constexpr (size == 4) {
        v = __builtin_bswap32(v);
      } else if constexpr (size == 2) {
        v = __builtin_bswap16(v);
      } else {
        UNREACHABLE;
      }
#endif

      putByte((uint8_t *)&v, size);
    } else {
      encode(utils::to_tuple(v));
    }
  }

  template <typename T, typename... Args>
  constexpr void encode(const T &t, const Args &...args) {
    encode(t);
    encode(args...);
  }

}  // namespace kagome::scale

#endif  // KAGOME_STRUCT_TO_TUPLE_HPP
