/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace kagome::common::literals {

  constexpr std::size_t operator""_kB(long long unsigned int kilobytes) {
    return static_cast<std::size_t>(kilobytes << 10u);
  }

  constexpr std::size_t operator""_kB(long double kilobytes) {
    return static_cast<std::size_t>((1ull << 10ull) * kilobytes);
  }

  constexpr std::size_t operator""_MB(long long unsigned int megabytes) {
    return static_cast<std::size_t>(megabytes << 20ull);
  }

  constexpr std::size_t operator""_MB(long double megabytes) {
    return static_cast<std::size_t>((1ull << 20ull) * megabytes);
  }

  constexpr std::size_t operator""_GB(long long unsigned int gigabytes) {
    return static_cast<std::size_t>(gigabytes << 30ull);
  }

  constexpr std::size_t operator""_GB(long double gigabytes) {
    return static_cast<std::size_t>((1ull << 30ull) * gigabytes);
  }

}  // namespace kagome::common::literals
