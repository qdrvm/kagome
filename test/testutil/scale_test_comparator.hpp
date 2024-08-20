/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <scale/scale.hpp>
#include "scale/kagome_scale.hpp"

namespace testutil {

  template <typename... T>
  inline outcome::result<std::vector<uint8_t>> scaleEncodeAndCompareWithRef(
      T &&...t) {
    return ::scale::encode(std::forward<T>(t)...);
  }

}  // namespace testutil
