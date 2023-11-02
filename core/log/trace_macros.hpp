/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "log/logger.hpp"

#include <sstream>

#include "common/hexutil.hpp"

namespace kagome::log {

  template <typename Ret, typename... Args>
  void trace_function_call(const Logger &logger,
                           std::string_view func_name,
                           Ret &&ret,
                           Args &&...args) {
    if (logger->level() >= Level::TRACE) {
      if (sizeof...(args) > 0) {
        std::string ss;
        (fmt::format_to(
             std::back_inserter(ss), "{}, ", std::forward<Args>(args)),
         ...);
        logger->trace("call '{}', args: {} -> ret: {}",
                      func_name,
                      ss,
                      std::forward<Ret>(ret));
      } else {
        logger->trace(
            "call '{}' -> ret: {}", func_name, std::forward<Ret>(ret));
      }
    }
  }

  template <typename... Args>
  void trace_void_function_call(const Logger &logger,
                                std::string_view func_name,
                                Args &&...args) {
    if (logger->level() >= Level::TRACE) {
      if (sizeof...(args) > 0) {
        std::string ss;
        (fmt::format_to(
             std::back_inserter(ss), "{}, ", std::forward<Args>(args)),
         ...);
        logger->trace("call '{}', args: {}", func_name, ss);
      } else {
        logger->trace("call '{}'", func_name);
      }
    }
  }

#ifdef NDEBUG

#define SL_TRACE_FUNC_CALL(logger, ret, ...)
#define SL_TRACE_VOID_FUNC_CALL(logger, ...)

#else

#define SL_TRACE_FUNC_CALL(logger, ret, ...) \
  ::kagome::log::trace_function_call(        \
      (logger), __FUNCTION__, (ret), ##__VA_ARGS__)

#define SL_TRACE_VOID_FUNC_CALL(logger, ...) \
  ::kagome::log::trace_void_function_call((logger), __FUNCTION__, ##__VA_ARGS__)

#endif

}  // namespace kagome::log
