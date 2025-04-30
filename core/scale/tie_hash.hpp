/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <scale/scale.hpp>
#include "utils/tuple_hash.hpp"

namespace scale {
  using detail::decompose_and_apply;

  size_t tieHash(const auto &value) {
    size_t seed = 0;

    auto append = [&]<typename T>(const T &element) {
      boost::hash_combine(seed, std::hash<std::remove_cvref_t<T>>()(element));
    };

    auto apply = [&](const auto &...elements) {  //
      (append(elements), ...);
    };

    decompose_and_apply(value, apply);

    return seed;
  }

}  // namespace scale

#define SCALE_TIE_HASH_BOOST(type)        \
  friend auto hash_value(const type &v) { \
    return ::scale::tieHash(v);           \
  }

#define SCALE_TIE_HASH_STD(type)             \
  template <>                                \
  struct std::hash<type> {                   \
    size_t operator()(const type &v) const { \
      return ::scale::tieHash(v);            \
    }                                        \
  }
