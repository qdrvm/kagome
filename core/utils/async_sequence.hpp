/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UTILS_ASYNC_SEQUENCE_HPP
#define KAGOME_UTILS_ASYNC_SEQUENCE_HPP

#include <type_traits>

#include "outcome/into.hpp"

namespace kagome {
  /**
   * Wrap synchronous function.
   */
  template <typename F>
  auto asAsync(F &&f) {
    return [f{std::forward<F>(f)}](auto &&next, auto &&...a) mutable {
      using Next = std::remove_cv_t<std::remove_reference_t<decltype(next)>>;
      return const_cast<Next &&>(next)(f(std::forward<decltype(a)>(a)...));
    };
  }

  /**
   * Chain async functions.
   */
  template <typename Fn>
  auto chainAsync(Fn &&fn) {
    return std::forward<Fn>(fn);
  }
  template <typename F1, typename... Fs>
  auto chainAsync(F1 &&f1, Fs &&...fs) {
    auto f2 = chainAsync(std::forward<Fs>(fs)...);
    return [f1{std::forward<F1>(f1)}, f2{std::move(f2)}](auto &&...a1) mutable {
      f1(f2, std::forward<decltype(a1)>(a1)...);
    };
  }

  /**
   * Wrap async function to accept value or propagate error.
   */
  template <typename F>
  auto mapAsyncOutcome(F &&f) {
    return [f{std::forward<F>(f)}](auto &&next, auto &&a) mutable {
      auto r = outcome::into(std::forward<decltype(a)>(a));
      if (r.has_value()) {
        f(std::forward<decltype(next)>(next), std::move(r.value()));
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
  Fn _sequence(Fn &&fn) {
    return std::forward<Fn>(fn);
  }
  template <typename F1, typename... Fs>
  auto _sequence(F1 &&f1, Fs &&...fs) {
    return chainAsync(mapAsyncOutcome(std::forward<F1>(f1)),
                      _sequence(std::forward<Fs>(fs)...));
  }

  /**
   * Call chain of async outcome functions.
   *
   * First function accepts no parameters.
   * Intermediate functions accept value.
   * Last function accepts `Result`.
   *
   * For handling intermediate errors use `chainAsync` and `mapAsyncOutcome`.
   */
  template <typename F1, typename... Fs>
  auto sequence(F1 &&f1, Fs &&...fs) {
    f1(_sequence(std::forward<Fs>(fs)...));
  }

  /**
   * Call chain of async outcome functions and ignore final result.
   *
   * First function accepts no parameters.
   * All other functions accept value.
   */
  template <typename... Fs>
  auto sequenceIgnore(Fs &&...fs) {
    sequence(std::forward<Fs>(fs)..., [](auto &&) {});
  }
}  // namespace kagome

#endif  // KAGOME_UTILS_ASYNC_SEQUENCE_HPP
