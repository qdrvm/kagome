/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_NO_FN_HPP
#define KAGOME_COMMON_NO_FN_HPP

#include <cstddef>

namespace kagome {
  /**
   * Explicitly specify empty `std::function` parameter.
   * All `std::function<F>` have `nullptr_t` constructor.
   *
   * Example:
   *   void foo(std::function<void()> optional_cb);
   *   foo(useful_cb);
   *   foo(kNoFn);
   */
  constexpr auto kNoFn = nullptr;
}  // namespace kagome

#endif  // KAGOME_COMMON_NO_FN_HPP
