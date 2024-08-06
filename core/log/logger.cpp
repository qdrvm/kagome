/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/assert.hpp>
#include <iostream>
#include <libp2p/log/logger.hpp>
#include <soralog/impl/sink_to_console.hpp>

#include "log/logger.hpp"
#include "log/profiling_logger.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::log, Error, e) {
  using E = kagome::log::Error;
  switch (e) {
    case E::WRONG_LEVEL:
      return "Unknown level";
    case E::WRONG_GROUP:
      return "Unknown group";
    case E::WRONG_LOGGER:
      return "Unknown logger";
  }
  return "Unknown log::Error";
}

namespace kagome::log {

  namespace {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    std::weak_ptr<soralog::LoggingSystem> logging_system_;

    std::shared_ptr<soralog::LoggingSystem>
    ensure_logger_system_is_initialized() {
      auto logging_system = logging_system_.lock();
      BOOST_ASSERT_MSG(
          logging_system,
          "Logging system is not ready. "
          "kagome::log::setLoggingSystem() must be executed once before");
      return logging_system;
    }
  }  // namespace

  outcome::result<Level> str2lvl(std::string_view str) {
    if (str == "trace") {
      return Level::TRACE;
    } else if (str == "debug") {
      return Level::DEBUG;
    } else if (str == "verbose") {
      return Level::VERBOSE;
    } else if (str == "info" or str == "inf") {
      return Level::INFO;
    } else if (str == "warning" or str == "warn") {
      return Level::WARN;
    } else if (str == "error" or str == "err") {
      return Level::ERROR;
    } else if (str == "critical" or str == "crit") {
      return Level::CRITICAL;
    } else if (str == "off" or str == "no") {
      return Level::OFF;
    } else {
      return Error::WRONG_LEVEL;
    }
  }

  void setLoggingSystem(std::weak_ptr<soralog::LoggingSystem> logging_system) {
    logging_system_ = logging_system;
    libp2p::log::setLoggingSystem(logging_system_.lock());
    profiling_logger = createLogger("Profiler", "profile");
  }

  void tuneLoggingSystem(const std::vector<std::string> &cfg) {
    auto logging_system = ensure_logger_system_is_initialized();

    if (cfg.empty()) {
      return;
    }

    for (auto &chunk : cfg) {
      if (auto res = str2lvl(chunk); res.has_value()) {
        auto level = res.value();
        setLevelOfGroup(kagome::log::defaultGroupName, level);
        continue;
      }

      std::istringstream iss2(chunk);

      std::string group_name;
      if (not std::getline(iss2, group_name, '=')) {
        std::cerr << "Can't read group";
      }
      if (not logging_system->getGroup(group_name)) {
        std::cerr << "Unknown group: " << group_name << std::endl;
        continue;
      }

      std::string level_string;
      if (not std::getline(iss2, level_string)) {
        std::cerr << "Can't read level for group '" << group_name << "'";
        continue;
      }
      auto res = str2lvl(level_string);
      if (not res.has_value()) {
        std::cerr << "Invalid level: " << level_string << std::endl;
        continue;
      }
      auto level = res.value();

      logging_system->setLevelOfGroup(group_name, level);
    }
  }

  Logger createLogger(const std::string &tag) {
    auto logging_system = ensure_logger_system_is_initialized();
    return std::static_pointer_cast<soralog::LoggerFactory>(logging_system)
        ->getLogger(tag, defaultGroupName);
  }

  Logger createLogger(const std::string &tag, const std::string &group) {
    auto logging_system = ensure_logger_system_is_initialized();
    return std::static_pointer_cast<soralog::LoggerFactory>(logging_system)
        ->getLogger(tag, group);
  }

  Logger createLogger(const std::string &tag,
                      const std::string &group,
                      Level level) {
    auto logging_system = ensure_logger_system_is_initialized();
    return std::static_pointer_cast<soralog::LoggerFactory>(logging_system)
        ->getLogger(tag, group, level);
  }

  bool setLevelOfGroup(const std::string &group_name, Level level) {
    auto logging_system = ensure_logger_system_is_initialized();
    return logging_system->setLevelOfGroup(group_name, level);
  }
  bool resetLevelOfGroup(const std::string &group_name) {
    auto logging_system = ensure_logger_system_is_initialized();
    return logging_system->resetLevelOfGroup(group_name);
  }

  bool setLevelOfLogger(const std::string &logger_name, Level level) {
    auto logging_system = ensure_logger_system_is_initialized();
    return logging_system->setLevelOfLogger(logger_name, level);
  }
  bool resetLevelOfLogger(const std::string &logger_name) {
    auto logging_system = ensure_logger_system_is_initialized();
    return logging_system->resetLevelOfLogger(logger_name);
  }

}  // namespace kagome::log
