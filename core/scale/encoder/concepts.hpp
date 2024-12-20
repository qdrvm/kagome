/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace kagome::scale {
  template <typename F>
  concept Invocable = std::is_invocable_v<F, const uint8_t *const, std::size_t>;

  template <typename T>
  concept IsEnum = std::is_enum_v<std::decay_t<T>>;

  template <typename T>
  concept IsNotEnum = !
  std::is_enum_v<std::decay_t<T>>;
}  // namespace kagome::scale
