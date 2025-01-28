/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "scale/kagome_scale.hpp"
#include "scale/scale.hpp"

namespace testutil {

  template <typename... T>
  inline outcome::result<std::vector<uint8_t>> scaleEncodeAndCompareWithRef(
      const T &...t) {
    return scale::encode(std::tie(t...));
  }

}  // namespace testutil
