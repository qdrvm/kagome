/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <string>

namespace kagome {
  inline auto &argv0() {
    static std::optional<std::string> executable;
    return executable;
  }
}  // namespace kagome
