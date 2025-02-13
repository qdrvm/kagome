/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "scale/kagome_scale.hpp"

namespace testutil {

  template <typename... Args>
  inline outcome::result<std::vector<uint8_t>> scaleEncodeAndCompareWithRef(
      const Args &...args) {
    return kagome::scale::encode(std::tie(args...));
  }

}  // namespace testutil
