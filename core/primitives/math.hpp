/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <bit>
#include <limits>
#include <type_traits>

namespace kagome::math {

  /**
   * Obtain closest multiple of X that is greater or equal to given number
   * @tparam X multiple that is POW of 2
   * @tparam T type of number
   * @param t given number
   * @return closest multiple
   */
  template <size_t X, typename T>
  inline constexpr T roundUp(T t) {
    static_assert((X & (X - 1)) == 0, "Must be POW 2!");
    static_assert(X != 0, "Must not be 0!");
    return (t + (X - 1)) & ~(X - 1);
  }

  template <typename T>
  inline constexpr T sat_sub_unsigned(T x, T y) {
    static_assert(std::numeric_limits<T>::is_integer
                      && !std::numeric_limits<T>::is_signed,
                  "Value must be integer and unsigned!");
    auto res = x - y;
    res &= -(res <= x);
    return res;
  }

  template <typename T,
            std::enable_if_t<std::is_integral_v<std::decay_t<T>>, bool> = true>
  constexpr auto toLE(const T &value) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    constexpr size_t size = sizeof(std::decay_t<T>);
    if constexpr (size == 8) {
      return __builtin_bswap64(value);
    } else if constexpr (size == 4) {
      return __builtin_bswap32(value);
    } else if constexpr (size == 2) {
      return __builtin_bswap16(value);
    }
#endif
    return value;
  }

  inline bool isPowerOf2(size_t x) {
    return ((x > 0ull) && ((x & (x - 1ull)) == 0));
  }

  inline size_t nextHighPowerOf2(size_t k) {
    if (isPowerOf2(k)) {
      return k;
    }
    auto n = std::numeric_limits<decltype(k)>::digits - std::countl_zero(k);
    return decltype(k){1} << n;
  }

}  // namespace kagome::math
