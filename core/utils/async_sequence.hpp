/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UTILS_ASYNC_SEQUENCE_HPP
#define KAGOME_UTILS_ASYNC_SEQUENCE_HPP

#include "outcome/into.hpp"

namespace kagome {
  /**
   * Chain async functions.
   */
  template <typename Fn>
  auto chainAsync(Fn fn) {
    return fn;
  }
  template <typename F1, typename... Fs>
  auto chainAsync(F1 f1, Fs... fs) {
    auto f2 = chainAsync(std::move(fs)...);
    return [f1{std::move(f1)}, f2{std::move(f2)}](auto... a1) mutable {
      f1(f2, std::forward<decltype(a1)>(a1)...);
    };
  }

  /**
   * Wrap async function to accept value or propagate error.
   */
  template <typename F>
  auto mapAsyncOutcome(F f) {
    return [f{std::move(f)}](auto next, auto a) mutable {
      auto r = outcome::Into<decltype(a)>::into(std::move(a));
      if (r.has_value()) {
        f(std::move(next), std::move(r.value()));
      } else {
        next(r.error());
      }
    };
  }

  /**
   * Workaround for `sequence` interface.
   *
   * `sequence(f1, ...fs, fn) == f1(chainAsync(mapAsyncOutcome(fs)..., fn))`
   */
  template <typename Fn>
  Fn _sequence(Fn fn) {
    return fn;
  }
  template <typename F1, typename... Fs>
  auto _sequence(F1 f1, Fs... fs) {
    return chainAsync(mapAsyncOutcome(std::move(f1)),
                      _sequence(std::move(fs)...));
  }

  /**
   * Call chain of async outcome functions.
   * First function accepts no parameters.
   * Intermediate functions accept value.
   * Last function accepts `Result`.
   *
   * For handling intermediate errors use `chainAsync` and `mapAsyncOutcome`.
   */
  template <typename F1, typename... Fs>
  auto sequence(F1 f1, Fs... fs) {
    f1(_sequence(std::move(fs)...));
  }
}  // namespace kagome

#endif  // KAGOME_UTILS_ASYNC_SEQUENCE_HPP
