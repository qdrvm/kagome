/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <span>

namespace kagome {
  /**
   * Rust `core::slice::from_raw_parts` requires non-null pointer,
   * but empty C++ `std::vector` returns null pointer.
   * https://doc.rust-lang.org/stable/core/slice/fn.from_raw_parts.html#safety
   * https://doc.rust-lang.org/stable/core/ptr/struct.NonNull.html#method.dangling
   */
  auto *nonNullDangling(auto &&t) {
    std::span s{t};
    using T = decltype(s)::element_type;
    if (not s.empty()) {
      return s.data();
    }
    return reinterpret_cast<T *>(alignof(T));
  }
}  // namespace kagome
