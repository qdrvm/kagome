/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <tuple>

namespace kagome {
  template <typename T>
  struct StdFunctionArgsTrait;

  template <typename R, typename... A>
  struct StdFunctionArgsTrait<std::function<R(A...)>> {
    using Args = std::tuple<A...>;
  };

  template <typename T>
  using StdFunctionArgs = typename StdFunctionArgsTrait<T>::Args;
}  // namespace kagome
