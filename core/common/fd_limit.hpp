/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_FD_LIMIT_HPP
#define KAGOME_COMMON_FD_LIMIT_HPP

#include <cstdlib>
#include <optional>

struct rlimit;

namespace kagome::common {
  std::optional<size_t> getFdLimit();
  void setFdLimit(size_t limit);
}  // namespace kagome::common

#endif  // KAGOME_COMMON_FD_LIMIT_HPP
