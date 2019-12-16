/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LOGGER_HPP
#define KAGOME_LOGGER_HPP

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

namespace kagome::common {
  using Logger = std::shared_ptr<spdlog::logger>;

  /**
   * Provide logger object
   * @param tag - tagging name for identifying logger
   * @return logger object
   */
  Logger createLogger(const std::string &tag);
}  // namespace kagome::common

#endif  // KAGOME_LOGGER_HPP
