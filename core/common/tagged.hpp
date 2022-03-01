/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TAGGED
#define KAGOME_TAGGED

#include <type_traits>

namespace kagome {

  /// Special sized wrapping type for differentiate some similar types
  template <typename T,
            typename Tag,
            typename = std::enable_if_t<std::is_scalar_v<T>>>
  class Tagged {
   public:
    explicit Tagged(T v) : v(v) {}

    operator T() const {
      return v;
    }

   private:
    typedef Tag tag;
    T v;
  };
}  // namespace kagome

#endif  // KAGOME_TAGGED
