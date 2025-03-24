/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/core.h>
#include <chrono>

namespace kagome {

  template <typename Rep, typename Period>
  struct pretty_duration {
    std::chrono::duration<Rep, Period> dur;
  };

  template <typename Rep, typename Period>
  pretty_duration(std::chrono::duration<Rep, Period>)
      -> pretty_duration<Rep, Period>;

  const char *suffix(unsigned denominator) {
    switch (denominator) {
      case 1:
        return "s";
      case 1000:
        return "ms";
      case 1'000'000:
        return "us";
      case 1'000'000'000:
        return "ns";
      default:
        return "??";
    }
  }

}  // namespace kagome

template <typename Rep, typename Period>
struct fmt::formatter<kagome::pretty_duration<Rep, Period>> {
  constexpr auto parse(format_parse_context &ctx)
      -> format_parse_context::iterator {
    return ctx.end();
  }

  auto format(const kagome::pretty_duration<Rep, Period> &p,
              format_context &ctx) const -> format_context::iterator {
    auto denominator = 1;
    static_assert(Period::num == 1);
    while (p.dur.count() / denominator > 1000 && denominator < Period::den) {
      denominator *= 1000;
    }
    return fmt::format_to(ctx.out(),
                          "{:.2f} {}",
                          static_cast<double>(p.dur.count()) / denominator,
                          kagome::suffix(Period::den / denominator));
  }
};
