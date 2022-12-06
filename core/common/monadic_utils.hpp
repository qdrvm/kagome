/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MONADIC_UTILS_H
#define KAGOME_MONADIC_UTILS_H

#include <optional>
#include <type_traits>

#include "outcome/outcome.hpp"

namespace kagome::common {

  /**
   * Applies \arg f to the value, stored in an optional.
   * @returns a new optional with the result of \arg f call, in case optional
   * contains a value. Otherwise, just returns an std::nullopt.
   */
  template <typename T,
            typename F,
            typename R = std::invoke_result_t<F, const T &>>
  std::optional<R> map_optional(std::optional<T> const &opt, F const &f) {
    if (opt.has_value()) {
      return std::optional<R>{f(opt.value())};
    }
    return std::nullopt;
  }

  /**
   * Applies \arg f to the value, stored in an optional.
   * @returns a new optional with the result of \arg f call, in case optional
   * contains a value. Otherwise, just returns an std::nullopt.
   */
  template <typename T, typename F, typename R = std::invoke_result_t<F, T &&>>
  std::optional<R> map_optional(std::optional<T> &&opt, F const &f) {
    if (opt.has_value()) {
      return std::optional<R>{f(std::move(opt.value()))};
    }
    return std::nullopt;
  }

  /**
   * Applies \arg f to the value, stored in an outcome::result.
   * @returns a new outcome::result with the result of \arg f call, in case
   * outcome::result contains a value. Otherwise, just returns the contained
   * error.
   */
  template <typename T,
            typename F,
            typename R = std::invoke_result_t<F, const T &>>
  outcome::result<R> map_result(outcome::result<T> const &res, F const &f) {
    if (res.has_value()) {
      return outcome::result<R>{f(res.value())};
    }
    return res.as_failure();
  }

  /**
   * Applies \arg f to the value, stored in an outcome::result.
   * @returns a new outcome::result with the result of \arg f call, in case
   * outcome::result contains a value. Otherwise, just returns the contained
   * error.
   */
  template <typename T, typename F, typename R = std::invoke_result_t<F, T &&>>
  outcome::result<R> map_result(outcome::result<T> &&res, F const &f) {
    if (res.has_value()) {
      return outcome::result<R>{f(std::move(res.value()))};
    }
    return res.as_failure();
  }

  /**
   * Applies \arg f to the value, stored in an optional wrapped in an
   * outcome::result.
   * @returns a new outcome::result of optional with the result of \arg f call,
   * in case outcome::result AND optional both contain a value. Otherwise, just
   * returns the contained error OR std::nullopt.
   */
  template <typename T,
            typename F,
            typename R = std::invoke_result_t<F, T const &>>
  outcome::result<std::optional<R>> map_result_optional(
      outcome::result<std::optional<T>> const &res_opt, F const &f) {
    return map_result(res_opt, [&f](auto &opt) {
      return map_optional(opt, [&f](auto &v) { return f(v); });
    });
  }

  /**
   * Applies \arg f to the value, stored in an optional wrapped in an
   * outcome::result.
   * @returns a new outcome::result of optional with the result of \arg f call,
   * in case outcome::result AND optional both contain a value. Otherwise, just
   * returns the contained error OR std::nullopt.
   */
  template <typename T, typename F, typename R = std::invoke_result_t<F, T &&>>
  outcome::result<std::optional<R>> map_result_optional(
      outcome::result<std::optional<T>> &&res_opt, F const &f) {
    return map_result(std::move(res_opt), [&f](std::optional<T> &&opt) {
      return map_optional(std::move(opt),
                          [&f](T &&v) { return f(std::move(v)); });
    });
  }

}  // namespace kagome::common

#endif  // KAGOME_MONADIC_UTILS_H
