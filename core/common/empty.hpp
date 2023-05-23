/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EMPTY
#define KAGOME_EMPTY

#include <scale/scale.hpp>

namespace kagome {

  /// Special zero-size-type for some things
  ///  (e.g., unsupported, experimental or empty).
  struct Empty {
    inline constexpr bool operator==(const Empty &) const {
      return true;
    }

    friend inline auto &operator<<(::scale::ScaleEncoderStream &s,
                                   const Empty &) {
      return s;
    }

    friend inline auto &operator>>(::scale::ScaleDecoderStream &s,
                                   const Empty &) {
      return s;
    }
  };

}  // namespace kagome

#endif  // KAGOME_EMPTY
