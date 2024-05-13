/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "log/logger.hpp"

namespace kagome::log {
  struct TraceReturnVoid {};

  template <typename Ret, typename Args>
  struct TraceFuncCall {
    const void *caller;
    std::string_view func_name;
    const Ret &ret;
    Args args;
  };
  template <typename Ret, typename Args>
  TraceFuncCall(const void *, std::string_view, const Ret &, Args)
      -> TraceFuncCall<Ret, Args>;

#ifdef NDEBUG

#define _SL_TRACE_FUNC_CALL(logger, ret, ...)

#else

#define _SL_TRACE_FUNC_CALL(logger, ret, ...) \
  SL_TRACE(logger,                            \
           "{}",                              \
           (::kagome::log::TraceFuncCall{     \
               this, __FUNCTION__, ret, std::forward_as_tuple(__VA_ARGS__)}))

#endif

}  // namespace kagome::log

template <typename Ret, typename Args>
struct fmt::formatter<kagome::log::TraceFuncCall<Ret, Args>> {
  static constexpr auto parse(format_parse_context &ctx) {
    return ctx.begin();
  }
  static auto format(const kagome::log::TraceFuncCall<Ret, Args> &v,
                     format_context &ctx) {
    auto out = ctx.out();
    out = fmt::format_to(out, "call '{}' from {}", v.func_name, v.caller);
    if constexpr ((std::tuple_size_v<Args>) > 0) {
      out = fmt::detail::write(out, ", args: ");
      auto first = true;
      auto f = [&](auto &arg) {
        if (first) {
          first = false;
        } else {
          out = fmt::detail::write(out, ", ");
        }
        out = fmt::format_to(out, "{}", arg);
      };
      std::apply([&](auto &...arg) { (f(arg), ...); }, v.args);
    }
    if constexpr (not std::is_same_v<Ret, kagome::log::TraceReturnVoid>) {
      out = fmt::format_to(out, " -> ret: {}", v.ret);
    }
    return out;
  }
};

#define SL_TRACE_FUNC_CALL(logger, ret, ...) \
  _SL_TRACE_FUNC_CALL(logger, ret, ##__VA_ARGS__)

#define SL_TRACE_VOID_FUNC_CALL(logger, ...) \
  _SL_TRACE_FUNC_CALL(logger, ::kagome::log::TraceReturnVoid{}, ##__VA_ARGS__)
