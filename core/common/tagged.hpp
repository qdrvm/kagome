/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TAGGED
#define KAGOME_TAGGED

#include <type_traits>
#include <utility>

namespace kagome {

  template <typename T, typename = std::enable_if<std::is_scalar_v<T>>>
  struct Wrapper {
    template <typename... Args>
    explicit Wrapper(Args &&...args) : value(std::forward<T>(args)...) {}

   protected:
    T value;
  };

  template <typename T,
            typename Tag,
            typename Base =
                std::conditional_t<std::is_scalar_v<T>, Wrapper<T>, T>>
  class Tagged final : public Base {
   public:
    typedef Tag tag;

    template <typename... Args>
    Tagged(Args &&...args) : Base(std::forward<Args>(args)...) {}

    template <typename Out>
    explicit operator Out() {
      if constexpr (std::is_scalar_v<T>) {
        return this->Wrapper<T>::value;
      } else {
        return *this;
      }
    }
  };
}  // namespace kagome

#endif  // KAGOME_TAGGED
