/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_BYTESTR_HPP
#define KAGOME_COMMON_BYTESTR_HPP

#include <cstdint>
#include <gsl/span>
#include <vector>

namespace kagome {
  inline gsl::span<const uint8_t> bytestr(gsl::span<const char> s) {
    return {(const uint8_t *)s.data(), s.size()};
  }

  inline std::vector<uint8_t> bytestr_copy(gsl::span<const char> s) {
    return {s.begin(), s.end()};
  }
}  // namespace kagome

#endif  // KAGOME_COMMON_BYTESTR_HPP
