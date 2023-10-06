/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_BYTESTR_HPP
#define KAGOME_COMMON_BYTESTR_HPP

#include <libp2p/common/bytestr.hpp>
#include <libp2p/common/span_size.hpp>
#include <string_view>
#include <vector>

namespace kagome {
  inline gsl::span<const uint8_t> str2byte(const gsl::span<const char> &s) {
    return libp2p::bytestr(s);
  }

  inline std::string_view byte2str(const gsl::span<const uint8_t> &s) {
    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const char *>(s.data()), libp2p::spanSize(s)};
  }

  inline std::vector<uint8_t> bytestr_copy(gsl::span<const char> s) {
    return {s.begin(), s.end()};
  }
}  // namespace kagome

#endif  // KAGOME_COMMON_BYTESTR_HPP
