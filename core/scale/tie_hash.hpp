/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "scale/tie.hpp"
#include "utils/tuple_hash.hpp"

namespace scale {
  auto tieHash(const auto &v) {
    return ::scale::as_tie(v,
                           [](auto t) { return std::hash<decltype(t)>{}(t); });
  }
}  // namespace scale

#define SCALE_TIE_HASH_BOOST(type) \
  friend auto hash_value(const type &v) { return ::scale::tieHash(v); }

#define SCALE_TIE_HASH_STD(type)                    \
  template <>                                       \
  struct std::hash<type> {                          \
    inline size_t operator()(const type &v) const { \
      return ::scale::tieHash(v);                   \
    }                                               \
  }
