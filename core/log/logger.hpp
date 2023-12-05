/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <sstream>

#include <soralog/level.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>
#include <soralog/macro.hpp>

#include "common/hexutil.hpp"
#include "outcome/outcome.hpp"

namespace kagome::log {

  using Level = soralog::Level;
  using Logger = std::shared_ptr<soralog::Logger>;
  using WLogger = std::weak_ptr<soralog::Logger>;

  enum class Error : uint8_t { WRONG_LEVEL = 1, WRONG_GROUP, WRONG_LOGGER };

  outcome::result<Level> str2lvl(std::string_view str);

  void setLoggingSystem(std::shared_ptr<soralog::LoggingSystem> logging_system);

  void tuneLoggingSystem(const std::vector<std::string> &cfg);

  static const std::string defaultGroupName("kagome");

  [[nodiscard]] Logger createLogger(const std::string &tag);

  [[nodiscard]] Logger createLogger(const std::string &tag,
                                    const std::string &group);

  [[nodiscard]] Logger createLogger(const std::string &tag,
                                    const std::string &group,
                                    Level level);

  bool setLevelOfGroup(const std::string &group_name, Level level);
  bool resetLevelOfGroup(const std::string &group_name);

  bool setLevelOfLogger(const std::string &logger_name, Level level);
  bool resetLevelOfLogger(const std::string &logger_name);

}  // namespace kagome::log

OUTCOME_HPP_DECLARE_ERROR(kagome::log, Error);
