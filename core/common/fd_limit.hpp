/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdlib>
#include <optional>

struct rlimit;

namespace kagome::common {
  std::optional<size_t> getFdLimit();
  void setFdLimit(size_t limit);
}  // namespace kagome::common
