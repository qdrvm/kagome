/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/assert.hpp>
#include <iostream>
#include <libp2p/log/logger.hpp>

#include "log/logger.hpp"
#include "log/profiling_logger.hpp"

namespace kagome::log {

  namespace {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    std::shared_ptr<soralog::LoggingSystem> logging_system_;

    void ensure_logger_system_is_initialized() noexcept {
      BOOST_ASSERT_MSG(
          logging_system_,
          "Logging system is not ready. "
          "kagome::log::setLoggingSystem() must be executed once before");
    }

    outcome::result<Level> str2lvl(std::string_view str) {
      if (str == "trace") {
        return Level::TRACE;
      } else if (str == "debug" or str == "warn") {
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
        return boost::system::error_code{};
      }
    }
  }  // namespace

  void setLoggingSystem(
      std::shared_ptr<soralog::LoggingSystem> logging_system) {
    BOOST_ASSERT(logging_system != nullptr);
    libp2p::log::setLoggingSystem(logging_system);
    logging_system_ = std::move(logging_system);
    profiling_logger = createLogger("Profiler", "profile");
  }

  void tuneLoggingSystem(const std::string &cfg) {
    ensure_logger_system_is_initialized();

    if (cfg.empty()) {
      return;
    }

    if (auto res = str2lvl(cfg); res.has_value()) {
      auto level = res.value();
      setLevelOfGroup(kagome::log::defaultGroupName, level);
      return;
    }

    std::istringstream iss(cfg);
    std::string chunk;
    while (std::getline(iss, chunk, ',')) {
      std::istringstream iss2(chunk);

      std::string group_name;
      if (not std::getline(iss2, group_name, '=')) {
        std::cerr << "Can't read group";
      }
      if (not logging_system_->getGroup(group_name)) {
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

      logging_system_->setLevelOfGroup(group_name, level);
    }
  }

  Logger createLogger(const std::string &tag) {
    ensure_logger_system_is_initialized();
    return std::dynamic_pointer_cast<soralog::LoggerFactory>(logging_system_)
        ->getLogger(tag, defaultGroupName);
  }

  Logger createLogger(const std::string &tag, const std::string &group) {
    ensure_logger_system_is_initialized();
    return std::dynamic_pointer_cast<soralog::LoggerFactory>(logging_system_)
        ->getLogger(tag, group);
  }

  Logger createLogger(const std::string &tag,
                      const std::string &group,
                      Level level) {
    ensure_logger_system_is_initialized();
    return std::dynamic_pointer_cast<soralog::LoggerFactory>(logging_system_)
        ->getLogger(tag, group, level);
  }

  void setLevelOfGroup(const std::string &group_name, Level level) {
    ensure_logger_system_is_initialized();
    logging_system_->setLevelOfGroup(group_name, level);
  }
  void resetLevelOfGroup(const std::string &group_name) {
    ensure_logger_system_is_initialized();
    logging_system_->resetLevelOfGroup(group_name);
  }

  void setLevelOfLogger(const std::string &logger_name, Level level) {
    ensure_logger_system_is_initialized();
    logging_system_->setLevelOfLogger(logger_name, level);
  }
  void resetLevelOfLogger(const std::string &logger_name) {
    ensure_logger_system_is_initialized();
    logging_system_->resetLevelOfLogger(logger_name);
  }

}  // namespace kagome::log
