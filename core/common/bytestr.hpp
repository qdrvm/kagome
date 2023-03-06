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
  using libp2p::bytestr;

  inline std::string_view bytestr(const gsl::span<const uint8_t> &s) {
    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const char *>(s.data()), libp2p::spanSize(s)};
  }
}  // namespace kagome

#endif  // KAGOME_COMMON_BYTESTR_HPP
