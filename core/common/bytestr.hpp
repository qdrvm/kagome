/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/bytestr.hpp>
#include <string_view>
#include <vector>

namespace kagome {
  inline std::span<const uint8_t> str2byte(const std::span<const char> &s) {
    return {reinterpret_cast<const uint8_t *>(s.data()), s.size()};
  }

  inline std::string_view byte2str(const std::span<const uint8_t> &s) {
    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const char *>(s.data()), s.size()};
  }

  inline std::vector<uint8_t> bytestr_copy(std::span<const char> s) {
    return {s.begin(), s.end()};
  }
}  // namespace kagome
