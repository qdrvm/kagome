/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LOGGER_HPP
#define KAGOME_LOGGER_HPP

#include <sstream>

#include <soralog/level.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>
#include <soralog/macro.hpp>

namespace kagome::log {

  using Level = soralog::Level;
  using Logger = std::shared_ptr<soralog::Logger>;

  void setLoggingSystem(std::shared_ptr<soralog::LoggingSystem> logging_system);

  [[nodiscard]] Logger createLogger(const std::string &tag);

  [[nodiscard]] Logger createLogger(const std::string &tag,
                                    const std::string &group);

  [[nodiscard]] Logger createLogger(const std::string &tag,
                                    const std::string &group,
                                    Level level);

  void setLevelOfGroup(const std::string &group_name, Level level);
  void resetLevelOfGroup(const std::string &group_name);

  void setLevelOfLogger(const std::string &logger_name, Level level);
  void resetLevelOfLogger(const std::string &logger_name);

  template <typename T, typename Ret>
  Ret format_arg(T const& t) {

  }

  template <typename T,
            typename = std::enable_if_t<std::is_integral_v<T>>>
  T const& format_arg(T const &arg) {
    return arg;
  }

  template <typename T,
            typename = std::enable_if_t<std::is_same_v<
                std::decay_t<decltype(*std::declval<T>().begin())>,
                uint8_t>>>
  std::string format_arg(T const &buffer) {
    if (buffer.size() == 0) return "";
    std::string res;
    if (isalnum(*buffer.begin()) && isalnum(*(buffer.end() - 1))) {
      res = buffer.toString();
    } else {
      res = buffer.toHex();
    }
    if (res.size() > 64) {
      return res.substr(0, 64);
    }
    return res;
  }

  template <typename Ret, typename... Args>
  void trace_function_call(Logger const &logger,
                           std::string_view func_name,
                           Ret &&ret,
                           Args &&...args) {
    if (sizeof...(args) > 0) {
      std::string ss;
      (fmt::format_to(std::back_inserter(ss),
                      "{}, ",
                      format_arg(std::forward<Args>(args))),
       ...);
      logger->trace("call func {}, args: {}, ret: {}",
                    func_name,
                    ss,
                    format_arg(std::forward<Ret>(ret)));
    } else {
      logger->trace("call func {}, ret: {}",
                    func_name,
                    format_arg(std::forward<Ret>(ret)));
    }
  }

  template <typename... Args>
  void trace_void_function_call(Logger const &logger,
                                std::string_view func_name,
                                Args &&...args) {
    if (sizeof...(args) > 0) {
      std::string ss;
      (fmt::format_to(std::back_inserter(ss),
                      "{}, ",
                      format_arg(std::forward<Args>(args))),
       ...);
      logger->trace("call func {}, args: {}", func_name, ss);
    } else {
      logger->trace("call func {}", func_name);
    }
  }

#ifndef NDEBUG

#define SL_TRACE_FUNC_CALL(logger, ret, ...) \
  ::kagome::log::trace_function_call(        \
      (logger), __FUNCTION__, (ret)__VA_OPT__(, ) __VA_ARGS__)

#define SL_TRACE_VOID_FUNC_CALL(logger, ...) \
  ::kagome::log::trace_void_function_call(   \
      (logger), __FUNCTION__ __VA_OPT__(, ) __VA_ARGS__)

#else

#define SL_TRACE_FUNC_CALL(logger, ret, ...)
#define SL_TRACE_VOID_FUNC_CALL(logger, ...)

#endif

}  // namespace kagome::log

#endif  // KAGOME_LOGGER_HPP
