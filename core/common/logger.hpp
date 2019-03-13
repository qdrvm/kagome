/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LOGGER_HPP
#define KAGOME_LOGGER_HPP

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

/// Allows to log objects, which have toString() method without calling it, e.g.
/// log.info("{}", myObject)
template <typename StreamType, typename T>
auto operator<<(StreamType &os, const T &object)
    -> decltype(os << object.toString()) {
  return os << object.toString();
}

namespace kagome::common::logger {
  using Logger = std::shared_ptr<spdlog::logger>;

  /**
   * Provide logger object
   * @param tag - tagging name for identifying logger
   * @return logger object
   */
  Logger createLogger(const std::string &tag);
}  // namespace kagome::common::logger

#endif  // KAGOME_LOGGER_HPP
