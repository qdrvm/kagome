/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <stdexcept>

namespace kagome {
  template <size_t I, typename T, typename... Ts>
  auto withType(size_t i, const auto &f) {
    if (i == I) {
      return f.template operator()<T>();
    }
    if constexpr (sizeof...(Ts) != 0) {
      return withType<I + 1, Ts...>(i, f);
    }
    throw std::out_of_range{"withType"};
  }

  /**
   * Calls `f` with `i`th type.
   */
  template <typename... T>
  auto withType(size_t i, const auto &f) {
    return withType<0, T...>(i, f);
  }

  /**
   * Wraps types for indexing.
   */
  template <typename... T>
  struct WithType {
    /**
     * Calls `withType` without specifying types explicitly.
     */
    static auto with(size_t i, const auto &f) {
      return withType<T...>(i, f);
    }
  };
}  // namespace kagome
