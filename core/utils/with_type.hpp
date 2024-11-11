/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>

namespace kagome {
  template <size_t I, typename T, typename... Ts>
  auto withType(size_t i, const auto &f) {
    if (i == I) {
      return f.template operator()<T>();
    }
    if constexpr (sizeof...(Ts) != 0) {
      return withType<I + 1, Ts...>(i, f);
    }
    abort();
  }

  template <typename... T>
  auto withType(size_t i, const auto &f) {
    return withType<0, T...>(i, f);
  }

  template <typename... T>
  struct WithType {
    static auto with(size_t i, const auto &f) {
      return withType<T...>(i, f);
    }
  };
}  // namespace kagome
