/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

namespace kagome {
  template <typename T>
  auto toSptr(T &&t) {
    return std::make_shared<std::decay_t<T>>(std::forward<T>(t));
  }
}  // namespace kagome
