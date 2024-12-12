/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <scale/scale.hpp>

namespace scale {
  /// Workaround for `encode` return type.
  auto encodeVec(const auto &v) {
    return scale::encode(v).value();
  }
}  // namespace scale
