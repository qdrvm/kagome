/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <scale/scale.hpp>

namespace kagome {

  /// Special zero-size-type for some things
  /// (e.g. unsupported, experimental or empty).
  struct Empty {
    inline constexpr bool operator==(const Empty &) const {
      return true;
    }

    template <class Stream>
    friend inline Stream &operator<<(Stream &s, const Empty &) {
      return s;
    }

    template <class Stream>
    friend inline Stream &operator>>(Stream &s, const Empty &) {
      return s;
    }
  };

}  // namespace kagome
