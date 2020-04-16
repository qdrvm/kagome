/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "common/logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace {
  void setGlobalPattern(spdlog::logger &logger) {
    logger.set_pattern("[%Y-%m-%d %H:%M:%S.%F] %n %v");
  }

  void setDebugPattern(spdlog::logger &logger) {
    logger.set_pattern("[%Y-%m-%d %H:%M:%S.%F][th:%t][%l] %n %v");
  }

  std::shared_ptr<spdlog::logger> createLogger(const std::string &tag,
                                               bool debug_mode = true) {
    auto logger = spdlog::stdout_color_mt(tag);
    if (debug_mode) {
      setDebugPattern(*logger);
    } else {
      setGlobalPattern(*logger);
    }
    return logger;
  }
}  // namespace

namespace kagome::common {
  Logger createLogger(const std::string &tag) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    auto logger = spdlog::get(tag);
    if (logger == nullptr) {
      logger = ::createLogger(tag);
    }
    return logger;
  }
}  // namespace kagome::common
