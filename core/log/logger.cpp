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

    template <typename... Args>
    std::shared_ptr<soralog::LoggingSystem> ensure_logger_system_is_initialized(
        const Args &...args) {
      auto logging_system = logging_system_.lock();
      if (logging_system == nullptr) {
        std::cerr
            << "Logging system is not ready. "
               "kagome::log::setLoggingSystem() must be executed once before. ";
        if constexpr (sizeof...(args) > 0) {
          std::cerr << '{';
          bool first = true;
          ((std::cerr << (first ? "" : ", ") << args, first = false), ...);
          std::cerr << '}' << std::endl;  // NOLINT(performance-avoid-endl)
        }
        std::abort();
      }
      return logging_system;
    }
  }  // namespace

  outcome::result<Level> str2lvl(std::string_view str) {
    if (str == "trace") {
      return Level::TRACE;
    }
    if (str == "debug") {
      return Level::DEBUG;
    }
    if (str == "verbose") {
      return Level::VERBOSE;
    }
    if (str == "info" or str == "inf") {
      return Level::INFO;
    }
    if (str == "warning" or str == "warn") {
      return Level::WARN;
    }
    if (str == "error" or str == "err") {
      return Level::ERROR;
    }
    if (str == "critical" or str == "crit") {
      return Level::CRITICAL;
    }
    if (str == "off" or str == "no") {
      return Level::OFF;
    }
    return Error::WRONG_LEVEL;
  }

  void setLoggingSystem(std::weak_ptr<soralog::LoggingSystem> logging_system) {
    logging_system_ = std::move(logging_system);
    libp2p::log::setLoggingSystem(logging_system_.lock());
    profiling_logger();  // call to create logger in advance
  }

  void tuneLoggingSystem(const std::vector<std::string> &cfg) {
    auto logging_system =
        ensure_logger_system_is_initialized("tuneLoggingSystem");

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
        std::cerr << "Unknown group: " << group_name
                  << std::endl;  // NOLINT(performance-avoid-endl)
        continue;
      }

      std::string level_string;
      if (not std::getline(iss2, level_string)) {
        std::cerr << "Can't read level for group '" << group_name << "'"
                  << std::endl;  // NOLINT(performance-avoid-endl)
        continue;
      }
      auto res = str2lvl(level_string);
      if (not res.has_value()) {
        std::cerr << "Invalid level: " << level_string
                  << std::endl;  // NOLINT(performance-avoid-endl)
        continue;
      }
      auto level = res.value();

      logging_system->setLevelOfGroup(group_name, level);
    }
  }

  void doLogRotate() {
    auto logging_system = ensure_logger_system_is_initialized();
    logging_system->callRotateForAllSinks();
  }

  Logger createLogger(const std::string &tag) {
    auto logging_system =
        ensure_logger_system_is_initialized("createLogger", tag);
    return std::static_pointer_cast<soralog::LoggerFactory>(logging_system)
        ->getLogger(tag, defaultGroupName);
  }

  Logger createLogger(const std::string &tag, const std::string &group) {
    auto logging_system =
        ensure_logger_system_is_initialized("createLogger", tag, group);
    return std::static_pointer_cast<soralog::LoggerFactory>(logging_system)
        ->getLogger(tag, group);
  }

  Logger createLogger(const std::string &tag,
                      const std::string &group,
                      Level level) {
    auto logging_system =
        ensure_logger_system_is_initialized("createLogger", tag, group);
    return std::static_pointer_cast<soralog::LoggerFactory>(logging_system)
        ->getLogger(tag, group, level);
  }

  bool setLevelOfGroup(const std::string &group_name, Level level) {
    auto logging_system =
        ensure_logger_system_is_initialized("setLevelOfGroup", group_name);
    return logging_system->setLevelOfGroup(group_name, level);
  }
  bool resetLevelOfGroup(const std::string &group_name) {
    auto logging_system =
        ensure_logger_system_is_initialized("resetLevelOfGroup", group_name);
    return logging_system->resetLevelOfGroup(group_name);
  }

  bool setLevelOfLogger(const std::string &logger_name, Level level) {
    auto logging_system =
        ensure_logger_system_is_initialized("setLevelOfLogger", logger_name);
    return logging_system->setLevelOfLogger(logger_name, level);
  }
  bool resetLevelOfLogger(const std::string &logger_name) {
    auto logging_system =
        ensure_logger_system_is_initialized("resetLevelOfLogger", logger_name);
    return logging_system->resetLevelOfLogger(logger_name);
  }

}  // namespace kagome::log
