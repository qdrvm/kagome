/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UNUSED
#define KAGOME_UNUSED

#include "common/empty.hpp"

namespace kagome {

  /// Number-based marker-type for using as tag
  template <size_t Num>
  struct NumTag {
   private:
    static constexpr size_t tag = Num;
  };

  /// Special zero-size-type for some things
  ///  (e.g., unsupported and experimental).
  template <size_t N>
  using Unused = Tagged<Empty, NumTag<N>>;

  template <size_t N>
  bool operator==(const Unused<N> &, const Unused<N> &) {
    return true;
  }

}  // namespace kagome

#endif  // KAGOME_UNUSED
