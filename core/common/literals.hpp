/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_LITERALS
#define KAGOME_COMMON_LITERALS

namespace kagome::common::literals {

  constexpr size_t operator""_kB(long long unsigned int kilobytes) {
    return kilobytes << 10u;
  }

  constexpr size_t operator""_kB(long double kilobytes) {
    return (1ull << 10) * kilobytes;
  }

  constexpr size_t operator""_MB(long long unsigned int megabytes) {
    return megabytes << 20u;
  }

  constexpr size_t operator""_MB(long double megabytes) {
    return (1ull << 20) * megabytes;
  }

  constexpr size_t operator""_GB(long long unsigned int gigabytes) {
    return gigabytes << 30u;
  }

  constexpr size_t operator""_GB(long double gigabytes) {
    return (1ull << 30) * gigabytes;
  }

}  // namespace kagome::common::literals

#endif  // KAGOME_COMMON_LITERALS
