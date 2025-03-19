/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <scale/detail/decomposable.hpp>

namespace scale {
  auto decomposeTie(const auto &value) {
    return ::scale::detail::decompose_and_apply(value, [](auto &&...a) {
      return std::tie(std::forward<decltype(a)>(a)...);
    });
  }
}  // namespace scale

#define SCALE_TIE_CMP(type)                                      \
  friend bool operator==(const type &l, const type &r) {         \
    return ::scale::decomposeTie(l) == ::scale::decomposeTie(r); \
  }                                                              \
  friend bool operator<(const type &l, const type &r) {          \
    return ::scale::decomposeTie(l) < ::scale::decomposeTie(r);  \
  }
