/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>
#include <vector>

#include "common/buffer.hpp"
#include "common/buffer_view.hpp"

namespace kagome {

  inline common::BufferView str2byte(std::span<const char> s) {
    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const uint8_t *>(s.data()), s.size()};
  }

  inline std::string_view byte2str(const common::BufferView &s) {
    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const char *>(s.data()), s.size()};
  }

  inline common::Buffer bytestr_copy(std::string_view s) {
    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return common::BufferView{reinterpret_cast<const uint8_t *>(s.data()),
                              s.size()};
  }

}  // namespace kagome
