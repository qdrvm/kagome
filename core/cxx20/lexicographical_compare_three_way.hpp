/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <compare>
#include <type_traits>

namespace cxx20 {
  /// https://en.cppreference.com/w/cpp/algorithm/lexicographical_compare_three_way
  template <class I1, class I2, class Cmp>
  constexpr auto lexicographical_compare_three_way(
      I1 f1, I1 l1, I2 f2, I2 l2, Cmp comp) -> decltype(comp(*f1, *f2)) {
    using ret_t = decltype(comp(*f1, *f2));
    static_assert(
        std::disjunction_v<std::is_same<ret_t, std::strong_ordering>,
                           std::is_same<ret_t, std::weak_ordering>,
                           std::is_same<ret_t, std::partial_ordering>>,
        "The return type must be a comparison category type.");

    bool exhaust1 = (f1 == l1);
    bool exhaust2 = (f2 == l2);
    for (; !exhaust1 && !exhaust2;
         exhaust1 = (++f1 == l1), exhaust2 = (++f2 == l2)) {
      if (auto c = comp(*f1, *f2); c != 0) {
        return c;
      }
    }

    return !exhaust1 ? std::strong_ordering::greater
         : !exhaust2 ? std::strong_ordering::less
                     : std::strong_ordering::equal;
  }
  template <class I1, class I2>
  constexpr auto lexicographical_compare_three_way(I1 f1, I1 l1, I2 f2, I2 l2) {
    return lexicographical_compare_three_way(
        f1, l1, f2, l2, std::compare_three_way{});
  }
}  // namespace cxx20
