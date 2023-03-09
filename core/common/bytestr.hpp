/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_BYTESTR_HPP
#define KAGOME_COMMON_BYTESTR_HPP

#include <libp2p/common/bytestr.hpp>
#include <libp2p/common/span_size.hpp>
#include <string_view>

namespace kagome {
  inline gsl::span<const uint8_t> str2byte(const gsl::span<const char> &s) {
    return libp2p::bytestr(s);
  }

  inline std::string_view byte2str(const gsl::span<const uint8_t> &s) {
    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const char *>(s.data()), libp2p::spanSize(s)};
  }
}  // namespace kagome

#endif  // KAGOME_COMMON_BYTESTR_HPP
