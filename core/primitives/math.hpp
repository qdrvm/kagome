/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MATH_HPP
#define KAGOME_MATH_HPP

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

}  // namespace kagome::math

#endif  // KAGOME_MATH_HPP
