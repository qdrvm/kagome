/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/empty.hpp>

namespace kagome {
  /// Special zero-size-type for some things
  /// (e.g. unsupported, experimental or empty).
  using qtils::Empty;
}  // namespace kagome

namespace qtils {
  // auxiliary definition fot gtest
  inline std::ostream &operator<<(std::ostream &s, const Empty &) {
    return s;
  }
}  // namespace qtils
