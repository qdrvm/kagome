/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EMPTY
#define KAGOME_EMPTY

namespace kagome {

  /// Special zero-size-type for some things
  ///  (e.g., unsupported, experimental or empty).
  struct Empty {
    constexpr bool operator==(const Empty &) const {
      return true;
    }
  };

  template <class Stream>
  Stream &operator<<(Stream &s, const Empty &) {
    return s;
  }

  template <class Stream>
  Stream &operator>>(Stream &s, Empty &) {
    return s;
  }

}  // namespace kagome

#endif  // KAGOME_EMPTY
