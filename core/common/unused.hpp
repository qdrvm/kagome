/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UNUSED
#define KAGOME_UNUSED

#include "common/empty.hpp"

namespace kagome {

  /// Special zero-size-type for some things
  ///  (e.g., unsupported and experimental).
  template <size_t N>
  struct Unused : public Empty {
    inline static constexpr size_t index = N;

    bool operator==(const Unused &) const {
      return true;
    }
  };

}  // namespace kagome

#endif  // KAGOME_UNUSED
