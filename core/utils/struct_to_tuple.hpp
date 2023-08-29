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
  auto to_tuple(T &&object) noexcept {
    using type = std::decay_t<T>;
    if constexpr (is_braces_constructible<type,
                                          any_type,
                                          any_type,
                                          any_type,
                                          any_type,
                                          any_type>{}) {
      auto &&[p1, p2, p3, p4, p5] = object;
      return std::make_tuple(p1, p2, p3, p4, p5);
    } else if constexpr (is_braces_constructible<type,
                                                 any_type,
                                                 any_type,
                                                 any_type,
                                                 any_type>{}) {
      auto &&[p1, p2, p3, p4] = object;
      return std::make_tuple(p1, p2, p3, p4);
    } else if constexpr (is_braces_constructible<type,
                                                 any_type,
                                                 any_type,
                                                 any_type>{}) {
      auto &&[p1, p2, p3] = object;
      return std::make_tuple(p1, p2, p3);
    } else if constexpr (is_braces_constructible<type, any_type, any_type>{}) {
      auto &&[p1, p2] = object;
      return std::make_tuple(p1, p2);
    } else if constexpr (is_braces_constructible<type, any_type>{}) {
      auto &&[p1] = object;
      return std::make_tuple(p1);
    } else {
      return std::make_tuple();
    }
  }
}  // namespace kagome::utils

namespace kagome::scale {

  template <typename T>
  outcome::result<void> encode(T &&v);

  template <typename... Ts>
  outcome::result<void> encode(std::tuple<Ts...> &&v);

  template <typename T, typename... Args>
  outcome::result<void> encode(T &&t, Args &&...args);

  void putByte(uint8_t val) {
    std::cout << std::hex << (uint32_t)val;
  }

  template <typename... Ts>
  outcome::result<void> encode(std::tuple<Ts...> &&v) {
    if constexpr (sizeof...(Ts) > 0) {
      std::apply([&](auto &&...s) { (..., encode(std::move(s))); },
                 std::move(v));
    }
    return outcome::success();
  }

  template <typename T>
  outcome::result<void> encode(T &&v) {
    using I = std::decay_t<T>;
    if constexpr (std::is_integral_v<I>) {
      if constexpr (std::is_same_v<I, bool>) {
        const uint8_t byte = (v ? 1u : 0u);
        putByte(byte);
        return outcome::success();
      }

      if constexpr (sizeof(I) == 1u) {
        putByte(static_cast<uint8_t>(v));
        return outcome::success();
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

      for (size_t i = 0; i < size; ++i) {
        putByte((uint8_t)((v >> (i * 8)) & 0xff));
      }
      return outcome::success();
    } else {
      return encode(utils::to_tuple(std::move(v)));
    }
  }

  template <typename T, typename... Args>
  outcome::result<void> encode(T &&t, Args &&...args) {
    OUTCOME_TRY(encode(std::forward<T>(t)));
    return encode(std::forward<Args>(args)...);
  }

}  // namespace kagome::scale

#endif  // KAGOME_STRUCT_TO_TUPLE_HPP
