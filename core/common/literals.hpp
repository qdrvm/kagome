/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace kagome::common::literals {

  constexpr size_t operator""_kB(long long unsigned int kilobytes) {
    return kilobytes << 10u;
  }

  constexpr size_t operator""_kB(long double kilobytes) {
    return static_cast<size_t>((1ull << 10ull) * kilobytes);
  }

  constexpr size_t operator""_MB(long long unsigned int megabytes) {
    return static_cast<size_t>(megabytes << 20ull);
  }

  constexpr size_t operator""_MB(long double megabytes) {
    return static_cast<size_t>((1ull << 20ull) * megabytes);
  }

  constexpr size_t operator""_GB(long long unsigned int gigabytes) {
    return static_cast<size_t>(gigabytes << 30ull);
  }

  constexpr size_t operator""_GB(long double gigabytes) {
    return static_cast<size_t>((1ull << 30ull) * gigabytes);
  }

}  // namespace kagome::common::literals
