/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "injector/lazy.hpp"

namespace testutil {

  template <typename T>
  struct CreatorSptr {
    template <typename R>
    R create() {
      return std::static_pointer_cast<typename R::element_type>(
          reinterpret_cast<T &>(*this));
    }
  };

  template <typename T, typename A = T>
  auto sptr_to_lazy(std::shared_ptr<A> &arg) {
    return kagome::LazySPtr<T>(
        reinterpret_cast<CreatorSptr<std::shared_ptr<A>> &>(arg));
  }

}  // namespace testutil
