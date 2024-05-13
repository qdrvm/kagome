/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "log/logger.hpp"

#include <boost/outcome/try.hpp>

namespace kagome::log {
  struct TraceReturnVoid {};

  template <typename Ret, typename... Args>
  void trace_function_call(const Logger &logger,
                           const void *caller,
                           std::string_view func_name,
                           Ret &&ret,
                           Args &&...args) {
    if (logger->level() >= Level::TRACE) {
      std::string str;
      auto to = std::back_inserter(str);
      fmt::format_to(to, "call '{}' from {}", func_name, caller);
      if constexpr (sizeof...(args) > 0) {
        fmt::format_to(to, ", args: ");
        (fmt::format_to(to, "{}, ", std::forward<Args>(args)), ...);
      }
      if constexpr (not std::is_same_v<Ret, TraceReturnVoid>) {
        fmt::format_to(to, " -> ret: {}", std::forward<Ret>(ret));
      }
      logger->trace("{}", str);
    }
  }

#ifdef NDEBUG

#define _SL_TRACE_FUNC_CALL(log_name, logger, ret, ...)

#else

#define _SL_TRACE_FUNC_CALL(log_name, logger, ret, ...)      \
  do {                                                       \
    auto &&log_name = (logger);                              \
    if (log_name->level() >= ::soralog::Level::TRACE) {      \
      ::kagome::log::trace_function_call(                    \
          log_name, this, __FUNCTION__, ret, ##__VA_ARGS__); \
    }                                                        \
  } while (false)

#endif

}  // namespace kagome::log

#define SL_TRACE_FUNC_CALL(logger, ret, ...) \
  _SL_TRACE_FUNC_CALL(BOOST_OUTCOME_TRY_UNIQUE_NAME, logger, ret, ##__VA_ARGS__)

#define SL_TRACE_VOID_FUNC_CALL(logger, ...)            \
  _SL_TRACE_FUNC_CALL(BOOST_OUTCOME_TRY_UNIQUE_NAME,    \
                      logger,                           \
                      ::kagome::log::TraceReturnVoid{}, \
                      ##__VA_ARGS__)
