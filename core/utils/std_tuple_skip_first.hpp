/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <tuple>

namespace kagome {
  template <typename T>
  struct StdTupleSkipFirstTrait;

  template <typename T, typename... Ts>
  struct StdTupleSkipFirstTrait<std::tuple<T, Ts...>> {
    using type = std::tuple<Ts...>;
  };

  template <typename T>
  using StdTupleSkipFirst = typename StdTupleSkipFirstTrait<T>::type;
}  // namespace kagome
