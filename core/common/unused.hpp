/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UNUSED
#define KAGOME_UNUSED

namespace kagome {

  /// Special zero-size-type for some things
  ///  (e.g., unsupported and experimental).
  template <size_t N>
  struct Unused {
    inline static constexpr size_t index = N;

    bool operator==(const Unused &) const {
      return true;
    }
  };

  template <size_t N, class Stream>
  Stream &operator<<(Stream &s, const Unused<N> &) {
    return s;
  }

  template <size_t N, class Stream>
  Stream &operator>>(Stream &s, Unused<N> &) {
    return s;
  }

}  // namespace kagome

#endif  // KAGOME_UNUSED
