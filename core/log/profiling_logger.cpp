/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log/profiling_logger.hpp"

namespace kagome::log {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  Logger profiling_logger() {
    static auto logger = createLogger("Profiler", "profile");
    return logger;
  }
}  // namespace kagome::log
